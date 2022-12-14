#include <windows.h>
#include <fstream>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "installwindow.h"
#include "ui_installwindow.h"

#include "copythread.h"
#include "hashthread.h"
#include "shellthread.h"

enum InstallStep
{
    HASH_CHECK,
    BACKUP_COPY,
    MAIN_COPY,
    SECONDARY_COPY,
    TUT_IMAGE_COPY,
    MOD_SETTINGS_UPDATE,
    COMPRESS,
    // hack to make compression take most of the bar
    COMPRESS_2,
    COMPRESS_3,
    COMPRESS_4,
    COMPRESS_5,
    CONFIG_APPEND,
    DONE
};

int GetInstallPercent(InstallStep step)
{
    return (100 * step) / InstallStep::DONE;
}

InstallWindow::InstallWindow(QString kenshiExePath, bool compressHeightmap, bool checkUpdates,
                             bool clearSkippedVersions, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InstallWindow)
{
    ui->setupUi(this);

    error = false;

    this->kenshiExePath = kenshiExePath;
    this->compressHeightmap = compressHeightmap;
    this->checkUpdates = checkUpdates;
    this->clearSkippedVersions = clearSkippedVersions;

    ui->label->setText("Double-checking hash...");

    HashThread *hashThread = new HashThread(kenshiExePath.toStdString());
    connect(hashThread, &HashThread::resultError, this, &InstallWindow::handleError);
    connect(hashThread, &HashThread::resultSuccess, this, &InstallWindow::handleExeHash);
    hashThread->start();
}

void InstallWindow::handleExeHash(QString hash)
{
    if(HashThread::HashIsModded(hash.toStdString()))
    {
        // undo EXE modifications
        ui->label->setText("Hash matches. Uninstalling old RE_Kenshi modifications...");
        // TODO
    }
    else if(HashThread::HashSupported(hash.toStdString()))
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
            ui->progressBar->setValue(GetInstallPercent(BACKUP_COPY));
        }
    }
    else
    {
        ui->label->setText("Hash doesn't match! This shouldn't be possible! No files changed, aborted. Mod not installed. It is now safe to close this window.");
        ui->progressBar->setValue(GetInstallPercent(DONE));
        return;
    }
}

void InstallWindow::handleBackupCopySuccess()
{
    QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
    std::string dllWritePath = kenshiDir.toStdString() + "RE_Kenshi.dll";
    ui->label->setText("Copying mod files...");
    CopyThread *modCopyThread = new CopyThread("tools/RE_Kenshi.dll", dllWritePath, this);
    connect(modCopyThread, &CopyThread::resultError, this, &InstallWindow::handleError);
    connect(modCopyThread, &CopyThread::resultSuccess, this, &InstallWindow::handleMainDLLCopySuccess);
    modCopyThread->start();
    ui->progressBar->setValue(GetInstallPercent(MAIN_COPY));
}

void InstallWindow::handleMainDLLCopySuccess()
{
    QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
    std::string dllWritePath = kenshiDir.toStdString() + "CompressToolsLib.dll";
    CopyThread *modCopyThread = new CopyThread("tools/CompressToolsLib.dll", dllWritePath, this);
    connect(modCopyThread, &CopyThread::resultError, this, &InstallWindow::handleError);
    connect(modCopyThread, &CopyThread::resultSuccess, this, &InstallWindow::handleSecondaryDLLCopySuccess);
    modCopyThread->start();
    ui->progressBar->setValue(GetInstallPercent(SECONDARY_COPY));
}

void InstallWindow::handleSecondaryDLLCopySuccess()
{

    QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
    // Create folder (done in-place as it should be instant)
    std::string command = "mkdir \"" + kenshiDir.toStdString() + "RE_Kenshi\"";
    // no error-check - checking if the tut file copy is successful is a better acid test
    system(command.c_str());
    std::string tutWritePath = kenshiDir.toStdString() + "RE_Kenshi/game_speed_tutorial.png";
    CopyThread *modCopyThread = new CopyThread("tools/game_speed_tutorial.png", tutWritePath, this);
    connect(modCopyThread, &CopyThread::resultError, this, &InstallWindow::handleError);
    connect(modCopyThread, &CopyThread::resultSuccess, this, &InstallWindow::handleTutorialImageCopySuccess);
    modCopyThread->start();
    ui->progressBar->setValue(GetInstallPercent(TUT_IMAGE_COPY));
}

void InstallWindow::handleTutorialImageCopySuccess()
{
    // no threading + GUI update on this once since it's fast
    ui->label->setText("Updating mod config...");
    QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
    QFile modConfigFile(kenshiDir + "RE_Kenshi.ini");
    modConfigFile.open(QFile::ReadOnly);
    QJsonDocument jsonDoc = QJsonDocument().fromJson(modConfigFile.readAll());
    modConfigFile.close();
    QJsonObject jsonObj = jsonDoc.object();
    jsonObj.insert("CheckUpdates", checkUpdates);
    // clear skipped version
    if(clearSkippedVersions)
        jsonObj.insert("SkippedVersion", "");
    jsonDoc.setObject(jsonObj);
    modConfigFile.open(QFile::WriteOnly);
    modConfigFile.write(jsonDoc.toJson());
    modConfigFile.close();

    ui->progressBar->setValue(GetInstallPercent(SECONDARY_COPY));
    // Go straight into the next block
    handleModSettingsUpdateSuccess();
}

void InstallWindow::handleModSettingsUpdateSuccess()
{
    if(compressHeightmap)
    {
        // TODO refactor - this is in two places
        ui->label->setText("Compressing heightmap, this may take a minute or two...");
        QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
        std::string heightmapReadPath = kenshiDir.toStdString() + "data/newland/land/fullmap.tif";
        std::string heightmapWritePath = kenshiDir.toStdString() + "data/newland/land/fullmap.cif";
        std::string command = "tools\\CompressTools.exe \"" + heightmapReadPath + "\" \"" + heightmapWritePath + "\"";
        ShellThread *kenshiModThread = new ShellThread(command);
        connect(kenshiModThread, &ShellThread::resultError, this, &InstallWindow::handleShellError);
        connect(kenshiModThread, &ShellThread::resultSuccess, this, &InstallWindow::handleHeightmapCompressSuccess);
        kenshiModThread->start();
        ui->progressBar->setValue(GetInstallPercent(COMPRESS));
    }
    else
    {
        // TODO refactor - this is in two places
        ui->label->setText("Adding RE_Kenshi to plugin config file...");
        QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
        std::string configWritePath = kenshiDir.toStdString() + "Plugins_x64.cfg";
        std::string pluginLoadStr = "Plugin=RE_Kenshi";
        std::string command = "find /c \"" + pluginLoadStr + "\" \"" + configWritePath + "\" >NUL || (echo. >> \"" + configWritePath + "\") && (echo " + pluginLoadStr + " >> \"" + configWritePath + "\")";
        ShellThread *kenshiModThread = new ShellThread(command);
        connect(kenshiModThread, &ShellThread::resultError, this, &InstallWindow::handleShellError);
        connect(kenshiModThread, &ShellThread::resultSuccess, this, &InstallWindow::handleConfigAppendSuccess);
        kenshiModThread->start();
        ui->progressBar->setValue(GetInstallPercent(CONFIG_APPEND));
    }
}

void InstallWindow::handleHeightmapCompressSuccess()
{
    // TODO refactor - this is in two places
    ui->label->setText("Adding RE_Kenshi to plugin config file...");
    QString kenshiDir = kenshiExePath.split("kenshi_GOG_x64.exe")[0].split("kenshi_x64.exe")[0];
    std::string configWritePath = kenshiDir.toStdString() + "Plugins_x64.cfg";
    std::string pluginLoadStr = "Plugin=RE_Kenshi";
    std::string command = "find /c \"" + pluginLoadStr + "\" \"" + configWritePath + "\" >NUL || (echo. >> \"" + configWritePath + "\") && (echo " + pluginLoadStr + " >> \"" + configWritePath + "\")";
    ShellThread *kenshiModThread = new ShellThread(command);
    connect(kenshiModThread, &ShellThread::resultError, this, &InstallWindow::handleShellError);
    connect(kenshiModThread, &ShellThread::resultSuccess, this, &InstallWindow::handleConfigAppendSuccess);
    kenshiModThread->start();
    ui->progressBar->setValue(GetInstallPercent(CONFIG_APPEND));
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
        ui->progressBar->setValue(GetInstallPercent(DONE));
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
