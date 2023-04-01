#include <windows.h>
#include <fstream>

#include "uninstallwindow.h"
#include "ui_installwindow.h"
#include "installwindow.h"

#include "copythread.h"
#include "hashthread.h"
#include "shellthread.h"
#include "diskutil.h"

#include "bugs.h"

enum UninstallStep
{
    CHECK_EXE_HASH,
    CHECK_BACKUP_EXE_HASH,
    RESTORE_CONFIG,
    RESTORE_EXE,
    DELETE_MAIN_DLL,
    DELETE_SECONDARY_DLL,
    DELETE_TUT_IMAGE,
    DELETE_HEIGHTMAP,
    DONE
};

int GetUninstallPercent(UninstallStep step)
{
    return (100 * step) / UninstallStep::DONE;
}

UninstallStep GetUninstallStepFromPercent(int percent)
{
    int currError = 100;
    int bestStep = -1;
    for(int i=0;i<=UninstallStep::DONE;++i)
    {
        if(std::abs(percent - GetUninstallPercent((UninstallStep)i)) < currError)
        {
            currError = std::abs(percent - GetUninstallPercent((UninstallStep)i));
            bestStep = i;
        }
    }
    return (UninstallStep)bestStep;
}

UninstallWindow::UninstallWindow(MainWindow::InstallerAction action, InstallOptions options, QWidget *parent)
    : QDialog(parent)
	, ui(new Ui::InstallWindow)
{
    ui->setupUi(this);

    error = false;

	this->options = options;
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

	HashThread *hashThread = new HashThread(options.kenshiExePath.toStdString());
    connect(hashThread, &HashThread::resultError, this, &UninstallWindow::handleError);
    connect(hashThread, &HashThread::resultSuccess, this, &UninstallWindow::handleExeHash);
    hashThread->start();
}

void UninstallWindow::handleExeHash(QString hash)
{
	QString kenshiDir = options.GetKenshiInstallDir();
    if(HashThread::HashIsModded(hash.toStdString()))
    {
        // undo EXE modifications
        ui->label->setText(tr("EXE hash matches old RE_Kenshi mod. Uninstalling old RE_Kenshi EXE modifications..."));
        std::string exeBackupPath = kenshiDir.toStdString() + "kenshi_x64_vanilla.exe";
        HashThread *hashThread = new HashThread(exeBackupPath);
        connect(hashThread, &HashThread::resultError, this, &UninstallWindow::handleError);
        connect(hashThread, &HashThread::resultSuccess, this, &UninstallWindow::handleBackupExeHash);
        hashThread->start();
        ui->progressBar->setValue(GetUninstallPercent(CHECK_BACKUP_EXE_HASH));
    }
    else if(HashThread::HashSupported(hash.toStdString()))
    {
        ui->label->setText(tr("EXE hash matches vanilla kenshi. Reverting kenshi plugin config to backup..."));
        std::string pluginsConfigPath = kenshiDir.toStdString() + "Plugins_x64.cfg";
        std::string pluginsConfigBackupPath = kenshiDir.toStdString() + "Plugins_x64_vanilla.cfg";
        std::ifstream configBackupFile(pluginsConfigBackupPath);
        if(configBackupFile.is_open())
        {
            CopyThread *configRestoreThread = new CopyThread(pluginsConfigBackupPath, pluginsConfigPath, this);
            connect(configRestoreThread, &CopyThread::resultError, this, &UninstallWindow::handleError);
            connect(configRestoreThread, &CopyThread::resultSuccess, this, &UninstallWindow::handleExeRestored);
            configRestoreThread->start();
            ui->progressBar->setValue(GetUninstallPercent(RESTORE_CONFIG));
        }
        else
        {
            ui->label->setText(tr("Critical error: no config file backup!"));
            Bugs::ReportBug("UninstallWindow", RESTORE_CONFIG, "Critical error: no config file backup!");
            ui->progressBar->setValue(GetUninstallPercent(DONE));
            return;
        }
    }
    else
    {
        ui->label->setText(tr("Hash doesn't match! This shouldn't be possible! No files changed, aborted. Mod not installed. It is now safe to close this window."));
        Bugs::ReportBug("UninstallWindow", CHECK_EXE_HASH, "Hash doesn't match!");
        ui->progressBar->setValue(GetUninstallPercent(DONE));
        return;
    }
}

void UninstallWindow::handleBackupExeHash(QString hash)
{
    if(HashThread::HashSupported(hash.toStdString()))
    {
        // restore backup
        ui->label->setText(tr("Backup hash matches, restoring vanilla kenshi EXE."));
		QString kenshiDir = options.GetKenshiInstallDir();
        std::string exeBackupPath = kenshiDir.toStdString() + "kenshi_x64_vanilla.exe";
		CopyThread *exeRestoreThread = new CopyThread(exeBackupPath, options.kenshiExePath.toStdString(), this);
        connect(exeRestoreThread, &CopyThread::resultError, this, &UninstallWindow::handleError);
        // RE_Kenshi 0.1-0.1.1 - don't need to revert plugin config file
        connect(exeRestoreThread, &CopyThread::resultSuccess, this, &UninstallWindow::handleExeRestored);
        exeRestoreThread->start();
        ui->progressBar->setValue(GetUninstallPercent(RESTORE_EXE));
    }
    else
    {
        ui->label->setText(tr("Backup EXE hash doesn't match! No files changed, aborted. Automatic uninstallation failed. It is now safe to close this window."));
        ui->progressBar->setValue(GetUninstallPercent(DONE));
        return;
    }
}

void UninstallWindow::handleExeRestored()
{
    ui->label->setText(tr("Old files restored. Removing RE_Kenshi DLL..."));
	QString kenshiDir = options.GetKenshiInstallDir();
    std::string command = "del \"" + kenshiDir.replace('/','\\').toStdString() + "RE_Kenshi.dll\"";
    ShellThread *dllDeleteThread = new ShellThread(command);
    connect(dllDeleteThread, &ShellThread::resultError, this, &UninstallWindow::handleShellError);
    connect(dllDeleteThread, &ShellThread::resultSuccess, this, &UninstallWindow::handleMainDLLDeleteSuccess);
    dllDeleteThread->start();
    ui->progressBar->setValue(GetUninstallPercent(DELETE_MAIN_DLL));
}

void UninstallWindow::handleMainDLLDeleteSuccess()
{
    ui->label->setText(tr("Old files restored. Removing CompressTools DLL..."));
	QString kenshiDir = options.GetKenshiInstallDir();
    std::string command = "del \"" + kenshiDir.replace('/','\\').toStdString() + "CompressToolsLib.dll\"";
    ShellThread *dllDeleteThread = new ShellThread(command);
    connect(dllDeleteThread, &ShellThread::resultError, this, &UninstallWindow::handleShellError);
    connect(dllDeleteThread, &ShellThread::resultSuccess, this, &UninstallWindow::handleSecondaryDLLDeleteSuccess);
    dllDeleteThread->start();
    ui->progressBar->setValue(GetUninstallPercent(DELETE_SECONDARY_DLL));
}

void UninstallWindow::handleSecondaryDLLDeleteSuccess()
{
    ui->label->setText(tr("Old files restored. Removing RE_Kenshi data..."));
	QString kenshiDir = options.GetKenshiInstallDir();
    std::string command = "del \"" + kenshiDir.replace('/','\\').toStdString() + "RE_Kenshi\\game_speed_tutorial.png\"";
    ShellThread *dllDeleteThread = new ShellThread(command);
    connect(dllDeleteThread, &ShellThread::resultError, this, &UninstallWindow::handleShellError);
    connect(dllDeleteThread, &ShellThread::resultSuccess, this, &UninstallWindow::handleTutorialImageDeleteSuccess);
    dllDeleteThread->start();
    ui->progressBar->setValue(GetUninstallPercent(DELETE_TUT_IMAGE));
}

void UninstallWindow::handleTutorialImageDeleteSuccess()
{
	QString kenshiDir = options.GetKenshiInstallDir();
    std::string heightmapPath = kenshiDir.replace('/','\\').toStdString() + "data\\newland\\land\\fullmap.cif";

    // check if compressed heightmap exists
    std::ifstream heightmapFile(heightmapPath);

    // If exists, delete
    if (heightmapFile.is_open())
    {
        heightmapFile.close();
        ui->label->setText(tr("Old files restored. Removing compressed heightmap..."));
        std::string command = "del \"" + heightmapPath + "\"";
        ShellThread *dllDeleteThread = new ShellThread(command);
        connect(dllDeleteThread, &ShellThread::resultError, this, &UninstallWindow::handleShellError);
        connect(dllDeleteThread, &ShellThread::resultSuccess, this, &UninstallWindow::handleSecondaryDLLDeleteSuccess);
        dllDeleteThread->start();
        ui->progressBar->setValue(GetUninstallPercent(DELETE_HEIGHTMAP));
    }
    // else, done
    else
    {
        handleCompressedHeightmapDeleteSuccess();
    }
}


void UninstallWindow::handleCompressedHeightmapDeleteSuccess()
{
    if(error)
    {
        Bugs::ReportBug("UninstallWindow", DONE, "UNCAUGHT ERROR: this should never happen");
        ui->label->setText(tr("UNCAUGHT ERROR?!? Sorry, I probably broke your kenshi install. Rename \"kenshi_x64_vanilla.exe\" to \"kenshi_x64.exe\" and \"Plugins_x64_vanilla.cfg\" to \"Plugins_x64.cfg\" to fix whatever I've done... :("));
    }
    else
    {
        ui->progressBar->setValue(GetUninstallPercent(DONE));

        if(action == MainWindow::UNINSTALL)
        {
            ui->label->setText(tr("RE_Kenshi has uninstalled successfully!"));
        }
        else if(action == MainWindow::UPGRADE)
        {
            // successfully uninstalled previous version, run installer to install new version
            this->hide();
			InstallWindow* nextWindow = new InstallWindow(options, nullptr);
            nextWindow->show();
        }
    }
    ui->closeButton->setEnabled(true);
}

void UninstallWindow::handleShellError(int errorVal)
{
    Bugs::ReportBug("UninstallWindow", GetUninstallStepFromPercent(ui->progressBar->value()), "Error: Shell command returned: " + std::to_string(errorVal));
    QString text = tr("Error: Shell command returned: ") + QString::number(errorVal) + tr(" install aborted.");
    ui->label->setText(text);
    this->error = true;
    ui->closeButton->setEnabled(true);
}

void UninstallWindow::handleError(QString errorStr)
{
    Bugs::ReportBug("UninstallWindow", GetUninstallStepFromPercent(ui->progressBar->value()), "Error: " + errorStr.toStdString());
    ui->label->setText(tr("Error: ") + errorStr);
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
