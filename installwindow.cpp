#include <windows.h>
#include <fstream>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

#include "installwindow.h"
#include "ui_installwindow.h"

#include "installthread.h"
#include "uninstallwindow.h"
#include "mainwindow.h"

#include "bugs.h"

InstallWindow::InstallWindow(InstallOptions  options, QWidget *parent, QString logs) :
    QDialog(parent),
	ui(new Ui::InstallWindow),
	log(logs)
{
    ui->setupUi(this);

	this->options = options;

	log += "INSTALL\n";

	thread = new InstallThread(this, options);
	connect(thread, &InstallThread::resultError, this, &InstallWindow::handleError);
	connect(thread, &InstallThread::resultCancel, this, &InstallWindow::handleCancel);
	connect(thread, &InstallThread::resultSuccess, this, &InstallWindow::handleSuccess);
	connect(thread, &InstallThread::progressUpdate, this, &InstallWindow::handleProgressUpdate);
	connect(thread, &InstallThread::statusUpdate, this, &InstallWindow::handleStatusUpdate);
	connect(thread, &InstallThread::log, this, &InstallWindow::handleLog);
	connect(thread, &InstallThread::reportBug, this, &InstallWindow::handleBugReport);
	connect(thread, &InstallThread::showMessageBox, this, &InstallWindow::showMessageBox);
	thread->start();
}

void InstallWindow::showMessageBox(QMessageBox* msgBox)
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

void InstallWindow::handleBugReport(int step, QString info)
{
	Bugs::ReportBug("Install", step, info.toStdString(), log.toStdString());
}

void InstallWindow::handleLog(QString text)
{
	// used for internal logging - sent with bug reports, not
	// normally shown to the user
	log += text + "\n";
}

void InstallWindow::handleStatusUpdate(QString text)
{
	// updates the status display text (also added to logs)
	ui->label->setText(text);
	log += text + "\n";
}

void InstallWindow::handleError(bool requireUninstall)
{
	// uninstall to unbork
	if(requireUninstall)
	{
		QMessageBox msgBox(QMessageBox::Warning, tr("Installation errored"), tr("An error occured while installing.")
						   + tr("\nAny copied files will now be uninstalled."),
						   QMessageBox::Ok);
		msgBox.exec();

		this->hide();
		UninstallWindow* uninstallWindow = new UninstallWindow(MainWindow::UNINSTALL, options);
		uninstallWindow->show();
	};
	ui->closeButton->setEnabled(true);
}

void InstallWindow::handleCancel(bool requireUninstall)
{
	// uninstall to unbork
	if(requireUninstall)
	{
		QMessageBox msgBox(QMessageBox::Warning, tr("Installation cancelled"), tr("Installation was cancelled.")
						   + tr("\nAny copied files will now be uninstalled."),
						   QMessageBox::Ok);
		msgBox.exec();

		this->hide();
		UninstallWindow* uninstallWindow = new UninstallWindow(MainWindow::UNINSTALL, options);
		uninstallWindow->show();
	}
	else
	{
		ui->label->setText(tr("Installation cancelled"));
		handleProgressUpdate(100);
	}

	ui->closeButton->setEnabled(true);
}

void InstallWindow::handleProgressUpdate(int percent)
{
	ui->progressBar->setValue(percent);
}

void InstallWindow::handleSuccess()
{
	ui->label->setText(tr("RE_Kenshi has installed successfully!"));
	ui->progressBar->setValue(100);
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
