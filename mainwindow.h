#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_kenshiDirButton_clicked();

    void on_nextButton_clicked();

    void on_kenshiDirText_textChanged(const QString &arg1);

    // install methods
    void handleExeHash(QString hash);
    void handleError(QString hash);

    // uninstall methods
    void handleBackupHash(QString hash);
    void handleUninstallFinish();
    void on_uninstallButton_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
