#include <windows.h>
#include <fstream>

#include "uninstallwindow.h"
#include "ui_installwindow.h"
#include "installwindow.h"

#include "uninstallthread.h"

#include "bugs.h"

#include <QFileInfo>
#include <QMessageBox>

UninstallWindow::UninstallWindow(MainWindow::InstallerAction action, InstallOptions options, QWidget *parent)
    : QDialog(parent)
	, ui(new Ui::InstallWindow)
{
    ui->setupUi(this);

	this->options = options;
    this->action = action;

	if(action == MainWindow::InstallerAction::UNINSTALL)
		log = "UNINSTALL\n";
	else if(action == MainWindow::InstallerAction::UPGRADE)
		log = "UPGRADE\n";
	else
		// TODO
		log = "BAD INSTALLER ACTION\n";

	thread = new UninstallThread(this, options);
	connect(thread, &UninstallThread::resultError, this, &UninstallWindow::handleError);
	connect(thread, &UninstallThread::resultCancel, this, &UninstallWindow::handleCancel);
	connect(thread, &UninstallThread::resultSuccess, this, &UninstallWindow::handleUninstallSuccess);
	connect(thread, &UninstallThread::progressUpdate, this, &UninstallWindow::handleProgressUpdate);
	connect(thread, &UninstallThread::statusUpdate, this, &UninstallWindow::handleStatusUpdate);
	connect(thread, &UninstallThread::log, this, &UninstallWindow::handleLog);
	connect(thread, &UninstallThread::reportBug, this, &UninstallWindow::handleBugReport);
	connect(thread, &UninstallThread::showMessageBox, this, &UninstallWindow::showMessageBox);
	thread->start();
}

void UninstallWindow::showMessageBox(QMessageBox* msgBox)
{
	// workaround for dumb threading problems
	QMessageBox localMsgBox;
	localMsgBox.setIcon(msgBox->icon());
	localMsgBox.setWindowTitle(msgBox->windowTitle());
	localMsgBox.setText(msgBox->text());
	localMsgBox.setStandardButtons(msgBox->standardButtons());
	localMsgBox.setDefaultButton(msgBox->defaultButton());
	msgBox->setResult(localMsgBox.exec());

	emit messageBoxFinished();
}

void UninstallWindow::handleBugReport(int step, QString info)
{
	Bugs::ReportBug("Uninstall", step, info.toStdString(), log.toStdString());
}

void UninstallWindow::handleLog(QString text)
{
	// used for internal logging - sent with bug reports, not
	// normally shown to the user
	log += text + "\n";
}

void UninstallWindow::handleStatusUpdate(QString text)
{
	// updates the status display text (also added to logs)
	ui->label->setText(text);
	log += text + "\n";
}

void UninstallWindow::handleUninstallSuccess()
{
	if(action == MainWindow::UNINSTALL)
	{
		// TODO report failed files here (don't need to on reinstall since they'll be
		// replaced by the installer)
		ui->label->setText(tr("RE_Kenshi has uninstalled successfully!"));
	}
	else if(action == MainWindow::UPGRADE)
	{
		// successfully uninstalled previous version, run installer to install new version
		this->hide();
		InstallWindow* nextWindow = new InstallWindow(options, nullptr, log);
		nextWindow->show();
	}

    ui->closeButton->setEnabled(true);
}

void UninstallWindow::handleError(bool modIsDisabled)
{
	QString labelText = ui->label->text();
	if(modIsDisabled)
		labelText += tr("\nRE_Kenshi has been disabled and may need reinstalling.");
	else
		labelText += tr("\nRE_Kenshi is probably still installed and functional.");

	ui->label->setText(labelText);
	ui->closeButton->setEnabled(true);

	// set progress to "DONE"
	handleProgressUpdate(100);
}

void UninstallWindow::handleCancel(bool modIsDisabled)
{
	QString label = tr("Uninstall cancelled.");
	if(modIsDisabled)
		label += tr("\nRE_Kenshi has been disabled and may need reinstalling.");
	else
		label += tr("\nRE_Kenshi is probably still installed and functional.");

	ui->label->setText(label);
	ui->closeButton->setEnabled(true);
}

void UninstallWindow::handleProgressUpdate(int percent)
{
	ui->progressBar->setValue(percent);
}

UninstallWindow::~UninstallWindow()
{
    delete ui;
}

void UninstallWindow::on_closeButton_clicked()
{
    this->close();
}

