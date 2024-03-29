#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTranslator>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum InstallerAction
    {
        UNINSTALL, // uninstall any previous version
        UPGRADE, // upgrade from previous version
        INSTALL // fresh install
    };

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
    void on_uninstallButton_clicked();

    void on_comboBox_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
	QTranslator mainTranslator;
	QTranslator baseTranslator;
};
#endif // MAINWINDOW_H
