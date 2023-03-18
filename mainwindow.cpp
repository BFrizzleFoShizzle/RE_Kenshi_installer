#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <fstream>
#include <qtranslator.h>

#include "hashthread.h"
#include "copythread.h"
#include "diskutil.h"

#include "optionswindow.h"
#include "uninstallwindow.h"
 #include <qlayout.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    translator.load(QString::fromStdString("./RE_Kenshi_" + QLocale::system().name().toStdString().substr(0,2)));
    QApplication::instance()->installTranslator(&translator);
    ui->setupUi(this);
    ui->comboBox->addItem("English", "en");
    std::string language = QLocale::system().name().toStdString().substr(0,2);
    ui->comboBox->setCurrentIndex(ui->comboBox->findData(QString::fromStdString(language)));

    // Dumb workaround to create a multiline button
    QGridLayout* layout = new QGridLayout(ui->kenshiDirButton);
    layout->addWidget(ui->kenshiDirButtonLabel);
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
    QString kenshiDir = kenshiEXEPath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
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
        ui->outputLabel->setText(tr("Hash ") + hash + tr(" does not match. This mod is only compatible with Kenshi 1.0.55, 1.0.59 (Steam) and 1.0.59 (GOG)."));
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
        UninstallWindow* uninstallWindow = new UninstallWindow(ui->kenshiDirText->text(), InstallerAction::UNINSTALL);
        uninstallWindow->show();
    }
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    QString language = QString(ui->comboBox->currentData().toString());
    qDebug(language.toStdString().c_str());
    QApplication::instance()->removeTranslator(&translator);
    translator.load("./RE_Kenshi_" + language);
    QApplication::instance()->installTranslator(&translator);
    ui->retranslateUi(this);
    // Apparently, have to do this manually...
    ui->outputLabel->setText(tr("Please select kenshi executable"));
    ui->kenshiDirButtonLabel->setText(tr("Find Kenshi install dir"));
    ui->nextButton->setText(tr("Next"));
    ui->uninstallButton->setText(tr("Uninstall"));
}
