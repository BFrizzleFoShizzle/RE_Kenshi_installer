#include "optionswindow.h"
#include "ui_optionswindow.h"
#include "installwindow.h"
#include "uninstallwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <fstream>

OptionsWindow::OptionsWindow(QString kenshiExePath, QMainWindow *previous, bool installToHDD, MainWindow::InstallerAction installerAction, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OptionsWindow)
    , backWindow(previous), kenshiExePath(kenshiExePath), action(installerAction)
{

    ui->setupUi(this);
    if(backWindow != nullptr)
        ui->backButton->setEnabled(true);

    QString label;
    if(installToHDD)
    {
        label = tr("Installing to HDD, heightmap compression enabled by default.\n") + label;
        ui->compressHeightmapCheckBox->setChecked(true);
    }
    else
    {
        label = tr("Not installing to HDD, heightmap compression disabled by default.\n");
        label += tr("Heightmap compression may or may not improve performance on your system.\n");
        ui->compressHeightmapCheckBox->setChecked(false);
    }

    label = ui->outputLabel->text() + "\n\n" + label;
    ui->outputLabel->setText(label);
}

OptionsWindow::~OptionsWindow()
{
    delete ui;
}

void OptionsWindow::on_nextButton_clicked()
{
    bool compressHeightmap = ui->compressHeightmapCheckBox->checkState();
    bool checkUpdates = ui->checkUpdatesCheckBox->checkState();
    bool clearSkippedVersions = ui->clearSkippedVersionsCheckBox->checkState();
    if(action == MainWindow::InstallerAction::UPGRADE)
    {
        // upgrade
        this->hide();
        UninstallWindow* uninstallWindow = new UninstallWindow(kenshiExePath, MainWindow::InstallerAction::UPGRADE, compressHeightmap, checkUpdates, clearSkippedVersions);
        uninstallWindow->show();
    }
    else
    {
        // regular install
        this->hide();
        InstallWindow* installWindow = new InstallWindow(kenshiExePath, compressHeightmap, checkUpdates, clearSkippedVersions);
        installWindow->show();
    }
}

void OptionsWindow::on_backButton_clicked()
{
    // TODO
    backWindow->show();
    this->close();
}
