#include <windows.h>
#include <fstream>

#include "installwindow.h"
#include "ui_installwindow.h"

#include "copythread.h"
#include "hashthread.h"
#include "shellthread.h"

InstallWindow::InstallWindow(QString kenshiExePath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InstallWindow)
{
    ui->setupUi(this);

    error = false;

    this->kenshiExePath = kenshiExePath;

    ui->label->setText("Double-checking hash...");

    HashThread *hashThread = new HashThread(kenshiExePath.toStdString());
    connect(hashThread, &HashThread::resultError, this, &InstallWindow::handleError);
    connect(hashThread, &HashThread::resultSuccess, this, &InstallWindow::handleExeHash);
    hashThread->start();
}

void InstallWindow::handleExeHash(QString hash)
{
    if(hash.toStdString() == moddedKenshiSteamHash)
    {
        // undo EXE modifications
        ui->label->setText("Hash matches. Uninstalling old RE_Kenshi modifications...");
        // TODO
    }
    else if(hash.toStdString() == vanillaKenshiSteamHash || hash.toStdString() == vanillaKenshiGOGHash)
    {
        ui->label->setText("Hash matches. Making kenshi plugin config backup...");
        QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
        std::string pluginsConfigPath = kenshiDir.toStdString() + "Plugins_x64.cfg";
        std::string pluginsConfigBackupPath = kenshiDir.toStdString() + "Plugins_x64_vanilla.cfg";
        std::ifstream configBackupFile(pluginsConfigBackupPath);
        if(configBackupFile.is_open())
        {
            ui->label->setText("Plugin config already backed up, skipping...");
            handleBackupCopySuccess();
        }
        else
        {
            CopyThread *exeBackupThread = new CopyThread(pluginsConfigPath, pluginsConfigBackupPath, this);
            connect(exeBackupThread, &CopyThread::resultError, this, &InstallWindow::handleError);
            connect(exeBackupThread, &CopyThread::resultSuccess, this, &InstallWindow::handleBackupCopySuccess);
            exeBackupThread->start();
            ui->progressBar->setValue(20);
        }
    }
    else
    {
        ui->label->setText("Hash doesn't match! This shouldn't be possible! No files changed, aborted. Mod not installed. It is now safe to close this window.");
        ui->progressBar->setValue(100);
        return;
    }
}

void InstallWindow::handleBackupCopySuccess()
{
    QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
    std::string dllWritePath = kenshiDir.toStdString() + "RE_Kenshi.dll";
    ui->label->setText("Copying mod files...");
    CopyThread *modCopyThread = new CopyThread("RE_Kenshi.dll", dllWritePath, this);
    connect(modCopyThread, &CopyThread::resultError, this, &InstallWindow::handleError);
    connect(modCopyThread, &CopyThread::resultSuccess, this, &InstallWindow::handleDLLCopySuccess);
    modCopyThread->start();
    ui->progressBar->setValue(40);
}

void InstallWindow::handleDLLCopySuccess()
{
    ui->label->setText("Adding RE_Kenshi to plugin config file...");
    QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
    std::string configWritePath = kenshiDir.toStdString() + "Plugins_x64.cfg";
    std::string pluginLoadStr = "Plugin=RE_Kenshi";
    std::string command = "find /c \"" + pluginLoadStr + "\" \"" + configWritePath + "\" >NUL || (echo. >> \"" + configWritePath + "\") && (echo " + pluginLoadStr + " >> \"" + configWritePath + "\")";
    ShellThread *kenshiModThread = new ShellThread(command);
    connect(kenshiModThread, &ShellThread::resultError, this, &InstallWindow::handleShellError);
    connect(kenshiModThread, &ShellThread::resultSuccess, this, &InstallWindow::handleConfigAppendSuccess);
    kenshiModThread->start();
    ui->progressBar->setValue(60);
}

void InstallWindow::handleConfigAppendSuccess()
{
    if(error)
    {
        ui->label->setText("UNCAUGHT ERROR?!? Sorry, I probably broke your kenshi install. Rename \"kenshi_x64_vanilla.exe\" to \"kenshi_x64.exe\" and \"Plugins_x64_vanilla.cfg\" to \"Plugins_x64.cfg\" to fix whatever I've done... :(");
    }
    else
    {
        ui->label->setText("RE_Kenshi has installed successfully!");
        ui->progressBar->setValue(100);
    }
    ui->closeButton->setEnabled(true);
}

void InstallWindow::handleShellError(int error)
{
    // TODO
    std::string text = "Error: Shell command returned: " + std::to_string(error) + " install aborted.";
    ui->label->setText(QString::fromStdString(text));
    error = true;
    ui->closeButton->setEnabled(true);
}

void InstallWindow::handleError(QString error)
{
    // TODO
    ui->label->setText("Error: " + error);
    error = true;
    ui->closeButton->setEnabled(true);
}

InstallWindow::~InstallWindow()
{
    delete ui;
}

void InstallWindow::on_closeButton_clicked()
{
    this->close();
}
