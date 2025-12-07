#ifndef OPTIONSWINDOW_H
#define OPTIONSWINDOW_H

#include <QMainWindow>
#include "mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class OptionsWindow; }
QT_END_NAMESPACE

class OptionsWindow : public QMainWindow
{
    Q_OBJECT

public:

	OptionsWindow(QString kenshiExePath, QMainWindow *previous, bool installToHDD, bool requiresDowngrade, MainWindow::InstallerAction installerAction, QWidget *parent = nullptr);
    ~OptionsWindow();

private slots:

    void on_backButton_clicked();
    void on_nextButton_clicked();

private:
    Ui::OptionsWindow *ui;
    QMainWindow *backWindow;

    QString kenshiExePath;
    MainWindow::InstallerAction action;
};

#endif // OPTIONSWINDOW_H
