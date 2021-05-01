#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <fstream>

#include "hashthread.h"
#include "copythread.h"

#include "installwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_kenshiDirButton_clicked()
{
    QString kenshiBinLoc = QFileDialog::getOpenFileName(this, "Select Kenshi executable:", "", "Kenshi executable (kenshi_x64.exe;kenshi_GOG_x64.exe)");
    ui->kenshiDirText->setText(kenshiBinLoc);
}

void MainWindow::on_nextButton_clicked()
{
    this->hide();
    InstallWindow* installWindow = new InstallWindow(ui->kenshiDirText->text());
    installWindow->show();
}

void MainWindow::on_kenshiDirText_textChanged(const QString &arg1)
{
    ui->uninstallButton->setEnabled(false);
    ui->nextButton->setEnabled(false);
    std::ifstream kenshiExe(arg1.toStdString(), std::ios::ate | std::ios::binary);
    if(kenshiExe.is_open())
    {
        ui->outputLabel->setText("Checking file hash...");

        HashThread *hashThread = new HashThread(arg1.toStdString(), this);
        // Hopefully stops ToCTToU race condition if you modify the path while hashes are being calculated
        ui->kenshiDirButton->setEnabled(false);
        ui->kenshiDirText->setEnabled(false);
        connect(hashThread, &HashThread::resultError, this, &MainWindow::handleError);
        connect(hashThread, &HashThread::resultSuccess, this, &MainWindow::handleExeHash);
        hashThread->start();
    }
    else
    {
        ui->outputLabel->setText("Please select kenshi executable.");
    }
}

void MainWindow::handleError(QString error)
{
    ui->outputLabel->setText("Error hashing file. " + error + " Please select kenshi executable.");
    ui->kenshiDirButton->setEnabled(true);
    ui->kenshiDirText->setEnabled(true);
}

void MainWindow::handleExeHash(QString hash)
{
    if(hash.toStdString() == vanillaKenshiGOGHash
            || hash.toStdString() == vanillaKenshiSteamHash
            || hash.toStdString() == moddedKenshiSteamHash)
    {
        ui->outputLabel->setText("Game hash matches. Continue...");
        ui->nextButton->setEnabled(true);
        ui->uninstallButton->setEnabled(false);
    }
    else
    {
        ui->outputLabel->setText("Hash " + hash + " does not match. This mod is only compatible with Kenshi 1.0.51 x64 (Steam/GOG).");
        ui->nextButton->setEnabled(false);
        ui->uninstallButton->setEnabled(false);
    }
    ui->kenshiDirButton->setEnabled(true);
    ui->kenshiDirText->setEnabled(true);
}

void MainWindow::on_uninstallButton_clicked()
{
    ui->kenshiDirButton->setEnabled(false);
    ui->kenshiDirText->setEnabled(false);
    ui->nextButton->setEnabled(false);
    ui->uninstallButton->setEnabled(false);
    QString kenshiLocation = ui->kenshiDirText->text();
    QMessageBox *confirmBox = new QMessageBox(this);
    confirmBox->setText("Are you sure you wish to uninstall RE_Kenshi?");
    confirmBox->addButton("Confirm", QMessageBox::ButtonRole::AcceptRole);
    confirmBox->addButton("Cancel", QMessageBox::ButtonRole::RejectRole);
    if(confirmBox->exec() == QMessageBox::ButtonRole::AcceptRole)
    {
        ui->outputLabel->setText("Checking backup hash...");
        /*
        QString kenshiLocation = ui->kenshiDirText->text();
        QString kenshiDir = kenshiLocation.split("kenshi_x64.exe")[0];
        std::string unmoddedExePath = kenshiDir.toStdString() + "kenshi_x64_vanilla.exe";
        HashThread *backupHashThread = new HashThread(unmoddedExePath, this);
        connect(backupHashThread, &HashThread::resultError, this, &MainWindow::handleError);
        connect(backupHashThread, &HashThread::resultSuccess, this, &MainWindow::handleBackupHash);
        backupHashThread->start();
        */
    }
}

void MainWindow::handleBackupHash(QString hash)
{
    // TODO
    /*
    if(hash.toStdString() == vanillaKenshiHash)
    {
        ui->outputLabel->setText("Hash matches. Continue...");
        QString kenshiLocation = ui->kenshiDirText->text();
        QString kenshiDir = kenshiLocation.split("kenshi_x64.exe")[0];
        std::string unmoddedExePath = kenshiDir.toStdString() + "kenshi_x64_vanilla.exe";
        CopyThread *backupCopy = new CopyThread(unmoddedExePath, kenshiLocation.toStdString(), this);
        connect(backupCopy, &CopyThread::resultError, this, &MainWindow::handleError);
        connect(backupCopy, &CopyThread::resultSuccess, this, &MainWindow::handleUninstallFinish);
        backupCopy->start();
    }
    else
    {
        ui->outputLabel->setText("Backup has incorrect hash... What?!? I probably broke your kenshi install. Rename \"kenshi_x64_vanilla.exe\" to \"kenshi_x64.exe\" to fix whatever I've done... :(");
    }
    */
}

void MainWindow::handleUninstallFinish()
{
    ui->outputLabel->setText("RE_Kenshi successfully uninstalled.");
}
