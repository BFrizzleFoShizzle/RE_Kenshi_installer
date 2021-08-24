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
    explicit UninstallWindow(QString kenshiExePath, MainWindow::InstallerAction action, bool compressHeightmap = false, QWidget *parent = nullptr);
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
    void handleCompressedHeightmapDeleteSuccess();
    QString kenshiExePath;
    bool error;
    MainWindow::InstallerAction action;
    bool compressHeightmap;
};

#endif // UNINSTALLWINDOW_H
