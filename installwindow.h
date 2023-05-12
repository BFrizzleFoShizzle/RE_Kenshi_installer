#ifndef INSTALLWINDOW_H
#define INSTALLWINDOW_H

#include <QDialog>

#include "installoptions.h"

class InstallThread;
class QMessageBox;

namespace Ui {
class InstallWindow;
}

class InstallWindow : public QDialog
{
    Q_OBJECT

public:
	explicit InstallWindow(InstallOptions  options, QWidget *parent = nullptr, QString logs = "");
    ~InstallWindow();

signals:
	// Note: this only works if MessageBoxes are submitted one at a time
	void messageBoxFinished();

private slots:
    void on_closeButton_clicked();

private:
	Ui::InstallWindow *ui;
	void handleError(bool requireUninstall);
	void handleCancel(bool requireUninstall);
	void handleSuccess();
	void handleProgressUpdate(int percent);
	void handleStatusUpdate(QString text);
	void handleLog(QString text);
	void handleBugReport(int step, QString info);
	void showMessageBox(QMessageBox* msgBox);

	InstallOptions options;
	InstallThread* thread;
	QString log;
};

#endif // INSTALLWINDOW_H
