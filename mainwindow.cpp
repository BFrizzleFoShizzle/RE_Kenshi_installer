#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include <fstream>
#include <qtranslator.h>

#include "hashthread.h"
#include "copythread.h"
#include "diskutil.h"

#include "optionswindow.h"
#include "uninstallwindow.h"
#include <qlayout.h>

std::vector<QString> requiredFiles = {"tools/RE_kenshi.dll",
						  "tools/CompressToolsLib.dll",
						  "tools/CompressTools.exe",
						  "tools/game_speed_tutorial.png",
						  "tools/locale",
						  "translations/qt_de.qm",
						  "translations/qt_de.qm",
						  "translations/RE_Kenshi_de.qm",
						  "translations/RE_Kenshi_ru.qm"};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
	QString language = QLocale::system().name().mid(0,2);
	baseTranslator.load("./translations/qt_" + language);
	QApplication::instance()->installTranslator(&baseTranslator);
	mainTranslator.load("./translations/RE_Kenshi_" + language);
	QApplication::instance()->installTranslator(&mainTranslator);
    ui->setupUi(this);
	ui->comboBox->addItem("English", "en");
	ui->comboBox->addItem("Deutsch", "de");
	ui->comboBox->addItem("Русский", "ru");
	ui->comboBox->setCurrentIndex(ui->comboBox->findData(language));

    // Dumb workaround to create a multiline button
    QGridLayout* layout = new QGridLayout(ui->kenshiDirButton);
    layout->addWidget(ui->kenshiDirButtonLabel);

	// autofill Kenshi install dir
	// Inspired by the OCS https://github.com/lmaydev/OpenConstructionSet/tree/main/OpenConstructionSet/Installations/Locators
	QString defaultPath = "";
	try
	{
		// GOG is easy - just read registry entry
		QSettings registryGOG("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\GOG.com\\Games\\1193046833", QSettings::NativeFormat);
		if(registryGOG.contains("exe"))
		{
			defaultPath = registryGOG.value("exe").toString();
		}
		// Steam is more compicated (and takes precedence over GOG)
		QSettings registrySteam("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Valve\\Steam", QSettings::NativeFormat);
		if(registrySteam.contains("InstallPath"))
		{
			QString steamInstallDir = registrySteam.value("InstallPath").toString();
			QStringList dirsToCheck;
			QFile libraryFolders(QDir(steamInstallDir).filePath("steamapps/libraryfolders.vdf"));
			// Tweaked from https://github.com/lmaydev/OpenConstructionSet/blob/main/OpenConstructionSet/Installations/Locators/SteamLocator.cs
			QRegularExpression  regex("^\\s+\"path\"\\s+\"(.+)\"");
			if (libraryFolders.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				while (!libraryFolders.atEnd()) {
					QString line = libraryFolders.readLine();
					QRegularExpressionMatch match = regex.match(line);
					if(match.hasMatch())
					{
						// "\\" should be the only path-valid escape code in VDF (which causes problems in shell commands, so we swap it out)
						dirsToCheck.append(match.captured(1).replace("\\\\", "\\"));
					}
				}
			}
			for(const QString& path : dirsToCheck)
			{
				QString testPath = QDir(path).filePath("steamapps/common/Kenshi/Kenshi_x64.exe");
				QFileInfo test_file(testPath);
				if(test_file.exists())
					defaultPath = testPath;
			}
		}
	}
	catch(...)
	{
		// do nothing
	}

	ui->kenshiDirText->setText(defaultPath);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_kenshiDirButton_clicked()
{
    QString kenshiBinLoc = QFileDialog::getOpenFileName(this, tr("Select Kenshi executable:"), "", "Kenshi executable (kenshi_x64.exe;kenshi_GOG_x64.exe)");
    ui->kenshiDirText->setText(kenshiBinLoc);
}

void MainWindow::on_nextButton_clicked()
{
	// check all required files exist
	for(QString file : requiredFiles)
	{
		QFileInfo check_file(file);

		if(!check_file.exists())
		{
			QMessageBox errorMessage(this);
			errorMessage.setWindowTitle(QObject::tr("Error"));
			errorMessage.setIcon(QMessageBox::Critical);
			errorMessage.setText(tr("Missing file: ") + file
						+ "\n" + tr("Make sure all files in the RE_Kenshi installer archive are extracted to the same folder as RE_Kenshi_installer.exe"));
			errorMessage.setStandardButtons(QMessageBox::Ok);
			errorMessage.exec();
			return;
		}
	}

    // HACK if uninstall button is active, previous version is installed, so we're doing an upgrade
    InstallerAction action = ui->uninstallButton->isEnabled() ? InstallerAction::UPGRADE : InstallerAction::INSTALL;

    // Checking if on HDD takes a while, so disable UI so nothing goes wrong
    // This is slightly nicer than closing the window (user can see stuff is still happening)
    bool oldUninstallEnabled = ui->uninstallButton->isEnabled();
    setEnabled(false);
    QString labelText = ui->outputLabel->text();
    ui->outputLabel->setText(labelText + "\n" + tr("Processing..."));
    repaint();

    std::string kenshiExePath = ui->kenshiDirText->text().toStdString();
    bool installToHDD = DiskUtil::IsOnHDD(kenshiExePath);

    this->hide();
    OptionsWindow* nextWindow = new OptionsWindow(ui->kenshiDirText->text(), this, installToHDD, action);
    nextWindow->show();

    // Re-enable UI in case user goes back to this window
    setEnabled(true);
    ui->outputLabel->setText(labelText);
}

void MainWindow::on_kenshiDirText_textChanged(const QString &arg1)
{
    ui->uninstallButton->setEnabled(false);
    ui->nextButton->setEnabled(false);
    std::ifstream kenshiExe(arg1.toStdString(), std::ios::ate | std::ios::binary);
    if(kenshiExe.is_open())
    {
        ui->outputLabel->setText(tr("Checking file hash..."));

        HashThread *hashThread = new HashThread(arg1.toStdString(), this);
        // Hopefully stops ToCTToU race condition if you modify the path while hashes are being calculated
        ui->kenshiDirButton->setEnabled(false);
        ui->kenshiDirText->setEnabled(false);
        connect(hashThread, &HashThread::resultError, this, &MainWindow::handleError);
        connect(hashThread, &HashThread::resultSuccess, this, &MainWindow::handleExeHash);
        hashThread->start();
    }
    else
    {
        ui->outputLabel->setText(tr("Please select kenshi executable."));
    }
}

void MainWindow::handleError(QString error)
{
    ui->outputLabel->setText(tr("Error hashing file. ") + error + tr(" Please select kenshi executable."));
    ui->kenshiDirButton->setEnabled(true);
    ui->kenshiDirText->setEnabled(true);
}


bool IsModInstalled(QString kenshiEXEHash, QString kenshiEXEPath)
{
    // if EXE is modded, mod is already installed :)
    if(HashThread::HashIsModded(kenshiEXEHash.toStdString()))
        return true;

    // if backup file + DLL exists, mod is installed
	QString kenshiDir = kenshiEXEPath.toLower().split("kenshi_gog_x64.exe")[0].split("kenshi_x64.exe")[0];
    std::string pluginsConfigBackupPath = kenshiDir.toStdString() + "Plugins_x64_vanilla.cfg";
    std::string dllPath = kenshiDir.toStdString() + "RE_Kenshi.dll";
    std::ifstream configBackupFile(pluginsConfigBackupPath);
    std::ifstream dllFile(dllPath);
    if(configBackupFile.is_open() && dllFile.is_open())
    {
        return true;
    }

    // if neither are true, mod is not installed
    return false;
}

void MainWindow::handleExeHash(QString hash)
{
    if(HashThread::HashSupported(hash.toStdString()))
    {
        ui->outputLabel->setText(tr("Game hash matches. Continue..."));
        ui->nextButton->setEnabled(true);
        ui->uninstallButton->setEnabled(false);

        // if mod is installed, enable uninstall
        if(IsModInstalled(hash, ui->kenshiDirText->text()))
            ui->uninstallButton->setEnabled(true);
    }
    else
    {
        ui->outputLabel->setText(tr("Hash ") + hash + tr(" does not match. This mod is only compatible with Kenshi ") + "1.0.55, 1.0.62, 1.0.64" + tr(" (Steam) and ") + "1.0.59" + tr(" (GOG)."));
        ui->nextButton->setEnabled(false);
        ui->uninstallButton->setEnabled(false);
    }
    ui->kenshiDirButton->setEnabled(true);
    ui->kenshiDirText->setEnabled(true);
}

void MainWindow::on_uninstallButton_clicked()
{
    ui->kenshiDirButton->setEnabled(false);
    ui->kenshiDirText->setEnabled(false);
    ui->nextButton->setEnabled(false);
    ui->uninstallButton->setEnabled(false);
    QString kenshiLocation = ui->kenshiDirText->text();
    QMessageBox *confirmBox = new QMessageBox(this);
    confirmBox->setText(tr("Are you sure you wish to uninstall RE_Kenshi?"));
    confirmBox->addButton(tr("Confirm"), QMessageBox::ButtonRole::AcceptRole);
    confirmBox->addButton(tr("Cancel"), QMessageBox::ButtonRole::RejectRole);
    if(confirmBox->exec() == QMessageBox::ButtonRole::AcceptRole)
    {
        this->hide();
		InstallOptions options;
		options.kenshiExePath = kenshiLocation;
		UninstallWindow* uninstallWindow = new UninstallWindow(InstallerAction::UNINSTALL, options);
        uninstallWindow->show();
    }
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
	QString language = QString(ui->comboBox->currentData().toString());
	QApplication::instance()->removeTranslator(&baseTranslator);
	baseTranslator.load("./translations/qt_" + language);
	QApplication::instance()->installTranslator(&baseTranslator);

	QApplication::instance()->removeTranslator(&mainTranslator);
	mainTranslator.load("./translations/RE_Kenshi_" + language);
	QApplication::instance()->installTranslator(&mainTranslator);

    ui->retranslateUi(this);
    // Apparently, have to do this manually...
    ui->outputLabel->setText(tr("Please select kenshi executable"));
    ui->kenshiDirButtonLabel->setText(tr("Find Kenshi install dir"));
    ui->nextButton->setText(tr("Next"));
    ui->uninstallButton->setText(tr("Uninstall"));
}
