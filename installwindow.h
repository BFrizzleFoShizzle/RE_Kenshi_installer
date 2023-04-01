#ifndef INSTALLWINDOW_H
#define INSTALLWINDOW_H

#include <QDialog>

#include "installoptions.h"
#include "copythread.h"

namespace Ui {
class InstallWindow;
}

class InstallWindow : public QDialog
{
    Q_OBJECT

public:
	explicit InstallWindow(InstallOptions  options, QWidget *parent = nullptr);
    ~InstallWindow();

private slots:
    void on_closeButton_clicked();

private:
    Ui::InstallWindow *ui;
    void handleError(QString error);
	void handleCancel();
    void handleShellError(int error);
    void handleExeHash(QString hash);
    void handleBackupCopySuccess();
    void handleMainDLLCopySuccess();
    void handleSecondaryDLLCopySuccess();
    void handleTutorialImageCopySuccess();
    void handleModSettingsUpdateSuccess();
    void handleHeightmapCompressSuccess();
    void handleConfigAppendSuccess();
	InstallOptions  options;
    bool error;
};

#endif // INSTALLWINDOW_H
