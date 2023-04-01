#ifndef UNINSTALLWINDOW_H
#define UNINSTALLWINDOW_H

#include <QDialog>
#include <mainwindow.h>
#include <installoptions.h>

namespace Ui {
class InstallWindow;
}

class UninstallWindow : public QDialog
{
    Q_OBJECT

public:
	explicit UninstallWindow(MainWindow::InstallerAction action, InstallOptions options, QWidget *parent = nullptr);
    ~UninstallWindow();

private slots:
    void on_closeButton_clicked();

private:
    Ui::InstallWindow *ui;
    void handleError(QString error);
    void handleShellError(int error);
    void handleExeHash(QString hash);
    void handleBackupExeHash(QString hash);
    void handleExeRestored();
    void handleMainDLLDeleteSuccess();
    void handleSecondaryDLLDeleteSuccess();
    void handleTutorialImageDeleteSuccess();
	void handleCompressedHeightmapDeleteSuccess();
	MainWindow::InstallerAction action;
	InstallOptions options;
	bool error;
};

#endif // UNINSTALLWINDOW_H
