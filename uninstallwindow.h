#ifndef UNINSTALLWINDOW_H
#define UNINSTALLWINDOW_H

#include <QDialog>
#include <mainwindow.h>

namespace Ui {
class InstallWindow;
}

class UninstallWindow : public QDialog
{
    Q_OBJECT

public:
    explicit UninstallWindow(QString kenshiExePath, MainWindow::InstallerAction action, QWidget *parent = nullptr);
    ~UninstallWindow();

private slots:
    void on_closeButton_clicked();

private:
    Ui::InstallWindow *ui;
    void handleError(QString error);
    void handleShellError(int error);
    void handleExeHash(QString hash);
    void handleBackupExeHash(QString hash);
    void handleFilesRestored();
    void handleDLLDeleteSuccess();
    QString kenshiExePath;
    bool error;
    MainWindow::InstallerAction action;
};

#endif // UNINSTALLWINDOW_H
