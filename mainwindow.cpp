#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <fstream>

#include "hashthread.h"
#include "copythread.h"

#include "installwindow.h"
#include "uninstallwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_kenshiDirButton_clicked()
{
    QString kenshiBinLoc = QFileDialog::getOpenFileName(this, "Select Kenshi executable:", "", "Kenshi executable (kenshi_x64.exe;kenshi_GOG_x64.exe)");
    ui->kenshiDirText->setText(kenshiBinLoc);
}

void MainWindow::on_nextButton_clicked()
{
    // HACK if uninstall button is active, previous version is installed, so we're doing an upgrade
    if(ui->uninstallButton->isEnabled())
    {
        // upgrade
        this->hide();
        UninstallWindow* uninstallWindow = new UninstallWindow(ui->kenshiDirText->text(), InstallerAction::UPGRADE);
        uninstallWindow->show();
    }
    else
    {
        // regular install
        this->hide();
        InstallWindow* installWindow = new InstallWindow(ui->kenshiDirText->text());
        installWindow->show();
    }
}

void MainWindow::on_kenshiDirText_textChanged(const QString &arg1)
{
    ui->uninstallButton->setEnabled(false);
    ui->nextButton->setEnabled(false);
    std::ifstream kenshiExe(arg1.toStdString(), std::ios::ate | std::ios::binary);
    if(kenshiExe.is_open())
    {
        ui->outputLabel->setText("Checking file hash...");

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
        ui->outputLabel->setText("Please select kenshi executable.");
    }
}

void MainWindow::handleError(QString error)
{
    ui->outputLabel->setText("Error hashing file. " + error + " Please select kenshi executable.");
    ui->kenshiDirButton->setEnabled(true);
    ui->kenshiDirText->setEnabled(true);
}


bool IsModInstalled(QString kenshiEXEHash, QString kenshiEXEPath)
{
    // if EXE is modded, mod is already installed :)
    if(kenshiEXEHash.toStdString() == moddedKenshiSteamHash)
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
    if(hash.toStdString() == vanillaKenshiGOGHash
            || hash.toStdString() == vanillaKenshiSteamHash
            || hash.toStdString() == moddedKenshiSteamHash)
    {
        ui->outputLabel->setText("Game hash matches. Continue...");
        ui->nextButton->setEnabled(true);
        ui->uninstallButton->setEnabled(false);

        // if mod is installed, enable uninstall
        if(IsModInstalled(hash, ui->kenshiDirText->text()))
            ui->uninstallButton->setEnabled(true);

    }
    else
    {
        ui->outputLabel->setText("Hash " + hash + " does not match. This mod is only compatible with Kenshi 1.0.51 x64 (Steam/GOG).");
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
    confirmBox->setText("Are you sure you wish to uninstall RE_Kenshi?");
    confirmBox->addButton("Confirm", QMessageBox::ButtonRole::AcceptRole);
    confirmBox->addButton("Cancel", QMessageBox::ButtonRole::RejectRole);
    if(confirmBox->exec() == QMessageBox::ButtonRole::AcceptRole)
    {
        this->hide();
        UninstallWindow* uninstallWindow = new UninstallWindow(ui->kenshiDirText->text(), InstallerAction::UNINSTALL);
        uninstallWindow->show();
    }
}
