#include "optionswindow.h"
#include "ui_optionswindow.h"
#include "installwindow.h"
#include "uninstallwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <fstream>

OptionsWindow::OptionsWindow(QString kenshiExePath, QMainWindow *previous, bool installToHDD, bool requiresDowngrade, MainWindow::InstallerAction installerAction, QWidget *parent)
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

	if(!requiresDowngrade)
	{
		// no downgrade, disable shortcut creation
		ui->createStartShortcutCheckBox->setVisible(false);
		ui->createStartShortcutCheckBox->setCheckState(Qt::Unchecked);
		ui->createDesktopShortcutCheckBox->setVisible(false);
		ui->createDesktopShortcutCheckBox->setCheckState(Qt::Unchecked);
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
	InstallOptions options;
	options.kenshiExePath = kenshiExePath;
	options.compressHeightmap = ui->compressHeightmapCheckBox->checkState();
	options.checkUpdates = ui->checkUpdatesCheckBox->checkState();
	options.clearSkippedVersions = ui->clearSkippedVersionsCheckBox->checkState();
	options.clearShaderCache = ui->clearShaderCacheCheckBox->checkState();
	options.createStartShortcut = ui->createStartShortcutCheckBox->checkState();
	options.createDesktopShortcut = ui->createDesktopShortcutCheckBox->checkState();

    if(action == MainWindow::InstallerAction::UPGRADE)
    {
        // upgrade
        this->hide();
		UninstallWindow* uninstallWindow = new UninstallWindow(MainWindow::InstallerAction::UPGRADE, options);
        uninstallWindow->show();
    }
    else
    {
        // regular install
        this->hide();
		InstallWindow* installWindow = new InstallWindow(options);
        installWindow->show();
    }
}

void OptionsWindow::on_backButton_clicked()
{
    // TODO
    backWindow->show();
    this->close();
}
