#ifndef INSTALLWINDOW_H
#define INSTALLWINDOW_H

#include <QDialog>

namespace Ui {
class InstallWindow;
}

class InstallWindow : public QDialog
{
    Q_OBJECT

public:
    explicit InstallWindow(QString kenshiExePath, QWidget *parent = nullptr);
    ~InstallWindow();

private slots:
    void on_closeButton_clicked();

private:
    Ui::InstallWindow *ui;
    void handleError(QString error);
    void handleShellError(int error);
    void handleExeHash(QString hash);
    void handleBackupCopySuccess();
    void handleDLLCopySuccess();
    void handleConfigAppendSuccess();
    QString kenshiExePath;
    bool error;
};

#endif // INSTALLWINDOW_H
