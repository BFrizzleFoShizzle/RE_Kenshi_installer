#ifndef UNINSTALLWINDOW_H
#define UNINSTALLWINDOW_H

#include <QDialog>
#include <mainwindow.h>
#include <installoptions.h>

class UninstallThread;
class QMessageBox;

namespace Ui {
class InstallWindow;
}

class UninstallWindow : public QDialog
{
    Q_OBJECT

public:
	explicit UninstallWindow(MainWindow::InstallerAction action, InstallOptions options, QWidget *parent = nullptr);
	~UninstallWindow();

signals:
	// Note: this only works if MessageBoxes are submitted one at a time
	void messageBoxFinished();

private slots:
	void on_closeButton_clicked();

private:
    Ui::InstallWindow *ui;
	void handleError(bool modIsDisabled);
	void handleCancel(bool installIsBroken);
	void handleProgressUpdate(int percent);
	void handleStatusUpdate(QString text);
	void handleLog(QString text);
	void handleBugReport(int step, QString info);
	void showMessageBox(QMessageBox* msgBox);
	void handleUninstallSuccess();
	MainWindow::InstallerAction action;
	InstallOptions options;
	UninstallThread *thread;
	QString log;
};

#endif // UNINSTALLWINDOW_H
