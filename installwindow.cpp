#include <windows.h>

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
    if(hash.toStdString() != vanillaKenshiHash)
    {
        ui->label->setText("Hash doesn't match! This shoudln't be possible! No files changed, aborted. Mod not installed. It is now safe to close this window.");
        ui->progressBar->setValue(100);
        return;
    }
    else
    {
        ui->label->setText("Hash matches. Making kenshi EXE backup...");
        QString kenshiDir = kenshiExePath.split("kenshi_x64.exe")[0];
        std::string exeBackupPath = kenshiDir.toStdString() + "kenshi_x64_vanilla.exe";

        CopyThread *exeBackupThread = new CopyThread(kenshiExePath.toStdString(), exeBackupPath, this);
        connect(exeBackupThread, &CopyThread::resultError, this, &InstallWindow::handleError);
        connect(exeBackupThread, &CopyThread::resultSuccess, this, &InstallWindow::handleBackupCopySuccess);
        exeBackupThread->start();
    }
}

void InstallWindow::handleBackupCopySuccess()
{
    QString kenshiDir = kenshiExePath.split("kenshi_x64.exe")[0];
    std::string dllWritePath = kenshiDir.toStdString() + "RE_Kenshi.dll";
    ui->label->setText("Copying mod files...");
    CopyThread *modCopyThread = new CopyThread("RE_Kenshi.dll", dllWritePath, this);
    connect(modCopyThread, &CopyThread::resultError, this, &InstallWindow::handleError);
    connect(modCopyThread, &CopyThread::resultSuccess, this, &InstallWindow::handleDLLCopySuccess);
    modCopyThread->start();
}

void InstallWindow::handleDLLCopySuccess()
{
    ui->label->setText("Modifying kenshi .exe");
    QString kenshiDir = kenshiExePath.split("kenshi_x64.exe")[0];
    std::string exeWritePath = kenshiDir.toStdString() + "kenshi_x64_modded.exe";
    std::string command = "bspatch \"" + kenshiExePath.toStdString() + "\" \"" + exeWritePath + "\" kenshi.patch";
    ShellThread *kenshiModThread = new ShellThread(command);
    connect(kenshiModThread, &ShellThread::resultError, this, &InstallWindow::handleShellError);
    connect(kenshiModThread, &ShellThread::resultSuccess, this, &InstallWindow::handleExeModSuccess);
    kenshiModThread->start();
    ui->label->setText(QString::fromStdString(command));
}

void InstallWindow::handleExeModSuccess()
{
    ui->label->setText("Replacing old EXE with modified EXE...");
    QString kenshiDir = kenshiExePath.split("kenshi_x64.exe")[0];
    std::string moddedExePath = kenshiDir.toStdString() + "kenshi_x64_modded.exe";
    CopyThread *modExeOverwriteThread = new CopyThread(moddedExePath, kenshiExePath.toStdString(), this);
    connect(modExeOverwriteThread, &CopyThread::resultError, this, &InstallWindow::handleError);
    connect(modExeOverwriteThread, &CopyThread::resultSuccess, this, &InstallWindow::handleExeOverwriteSuccess);
    modExeOverwriteThread->start();
}

void InstallWindow::handleExeOverwriteSuccess()
{
    if(error)
    {
        ui->label->setText("UNCAUGHT ERROR?!? Sorry, I probably broke your kenshi install. Rename \"kenshi_x64_vanilla.exe\" to \"kenshi_x64.exe\" to fix whatever I've done... :(");
    }
    else
    {
        ui->label->setText("RE_Kenshi has installed successfully!");
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
