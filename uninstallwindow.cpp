#include <windows.h>
#include <fstream>

#include "uninstallwindow.h"
#include "ui_installwindow.h"
#include "installwindow.h"

#include "copythread.h"
#include "hashthread.h"
#include "shellthread.h"

UninstallWindow::UninstallWindow(QString kenshiExePath, MainWindow::InstallerAction action, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InstallWindow)
{
    ui->setupUi(this);

    error = false;

    this->kenshiExePath = kenshiExePath;
    this->action = action;

    // Step 1: check if EXE is modded
    // If modded, restore backup
    // Step 2: check if RE_Kenshi is in plugin config file
    // If so, restore backup
    // Step 3: check if DLL exists
    // If so, remove
    // Step 4: check if config files exist
    // If so, ???

    ui->label->setText("Double-checking hash...");

    HashThread *hashThread = new HashThread(kenshiExePath.toStdString());
    connect(hashThread, &HashThread::resultError, this, &UninstallWindow::handleError);
    connect(hashThread, &HashThread::resultSuccess, this, &UninstallWindow::handleExeHash);
    hashThread->start();
}

void UninstallWindow::handleExeHash(QString hash)
{
    QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
    if(hash.toStdString() == moddedKenshiSteamHash)
    {
        // undo EXE modifications
        ui->label->setText("EXE hash matches old RE_Kenshi mod. Uninstalling old RE_Kenshi EXE modifications...");
        std::string exeBackupPath = kenshiDir.toStdString() + "kenshi_x64_vanilla.exe";
        HashThread *hashThread = new HashThread(exeBackupPath);
        connect(hashThread, &HashThread::resultError, this, &UninstallWindow::handleError);
        connect(hashThread, &HashThread::resultSuccess, this, &UninstallWindow::handleBackupExeHash);
        hashThread->start();
        ui->progressBar->setValue(20);
    }
    else if(hash.toStdString() == vanillaKenshiSteamHash || hash.toStdString() == vanillaKenshiGOGHash)
    {
        ui->label->setText("EXE hash matches vanilla kenshi. Reverting kenshi plugin config to backup...");
        std::string pluginsConfigPath = kenshiDir.toStdString() + "Plugins_x64.cfg";
        std::string pluginsConfigBackupPath = kenshiDir.toStdString() + "Plugins_x64_vanilla.cfg";
        std::ifstream configBackupFile(pluginsConfigBackupPath);
        if(configBackupFile.is_open())
        {
            CopyThread *configRestoreThread = new CopyThread(pluginsConfigBackupPath, pluginsConfigPath, this);
            connect(configRestoreThread, &CopyThread::resultError, this, &UninstallWindow::handleError);
            connect(configRestoreThread, &CopyThread::resultSuccess, this, &UninstallWindow::handleFilesRestored);
            configRestoreThread->start();
            ui->progressBar->setValue(20);
        }
        else
        {
            ui->label->setText("Critical error: no config file backup!");
            ui->progressBar->setValue(100);
            return;
        }
    }
    else
    {
        ui->label->setText("Hash doesn't match! This shouldn't be possible! No files changed, aborted. Mod not installed. It is now safe to close this window.");
        ui->progressBar->setValue(100);
        return;
    }
}

void UninstallWindow::handleBackupExeHash(QString hash)
{
    if(hash.toStdString() == vanillaKenshiSteamHash)
    {
        // restore backup
        ui->label->setText("Backup hash matches, restoring vanilla kenshi EXE.");
        QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
        std::string exeBackupPath = kenshiDir.toStdString() + "kenshi_x64_vanilla.exe";
        CopyThread *exeRestoreThread = new CopyThread(exeBackupPath, kenshiExePath.toStdString(), this);
        connect(exeRestoreThread, &CopyThread::resultError, this, &UninstallWindow::handleError);
        // RE_Kenshi 0.1-0.1.1 - don't need to revert plugin config file
        connect(exeRestoreThread, &CopyThread::resultSuccess, this, &UninstallWindow::handleFilesRestored);
        exeRestoreThread->start();
        ui->progressBar->setValue(40);
    }
    else
    {
        ui->label->setText("Backup EXE hash doesn't match! No files changed, aborted. Automatic uninstallation failed. It is now safe to close this window.");
        ui->progressBar->setValue(100);
        return;
    }
}

void UninstallWindow::handleFilesRestored()
{
    ui->label->setText("Old files restored. Removing RE_Kenshi DLL...");
    QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
    std::string command = "del \"" + kenshiDir.replace('/','\\').toStdString() + "RE_Kenshi.dll\"";
    ShellThread *dllDeleteThread = new ShellThread(command);
    connect(dllDeleteThread, &ShellThread::resultError, this, &UninstallWindow::handleShellError);
    connect(dllDeleteThread, &ShellThread::resultSuccess, this, &UninstallWindow::handleDLLDeleteSuccess);
    dllDeleteThread->start();
    ui->progressBar->setValue(60);
}

void UninstallWindow::handleDLLDeleteSuccess()
{
    if(error)
    {
        ui->label->setText("UNCAUGHT ERROR?!? Sorry, I probably broke your kenshi install. Rename \"kenshi_x64_vanilla.exe\" to \"kenshi_x64.exe\" and \"Plugins_x64_vanilla.cfg\" to \"Plugins_x64.cfg\" to fix whatever I've done... :(");
    }
    else
    {
        if(action == MainWindow::UNINSTALL)
        {
            ui->label->setText("RE_Kenshi has uninstalled successfully!");
            ui->progressBar->setValue(100);
        }
        else if(action == MainWindow::UPGRADE)
        {
            // successfully uninstalled previous version, run installer to install new version
            this->hide();
            InstallWindow* installWindow = new InstallWindow(kenshiExePath);
            installWindow->show();
        }
    }
    ui->closeButton->setEnabled(true);
}

void UninstallWindow::handleShellError(int error)
{
    // TODO
    std::string text = "Error: Shell command returned: " + std::to_string(error) + " install aborted.";
    ui->label->setText(QString::fromStdString(text));
    this->error = true;
    ui->closeButton->setEnabled(true);
}

void UninstallWindow::handleError(QString error)
{
    // TODO
    ui->label->setText("Error: " + error);
    this->error = true;
    ui->closeButton->setEnabled(true);
}

UninstallWindow::~UninstallWindow()
{
    delete ui;
}

void UninstallWindow::on_closeButton_clicked()
{
    this->close();
}
