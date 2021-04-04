#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <fstream>

#include "hashthread.h"

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
    QString kenshiBinLoc = QFileDialog::getOpenFileName(this, "Select Kenshi executable:", "", "kenshi_x64.exe");
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
        ui->nextButton->setEnabled(false);
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
    if(hash.toStdString() == vanillaKenshiHash)
    {
        ui->outputLabel->setText("Hash matches. Continue...");
        ui->nextButton->setEnabled(true);
    }
    else if(hash.toStdString() == moddedKenshiHash)
    {
        ui->outputLabel->setText("Mod is already enabled.");
        ui->nextButton->setEnabled(false);
    }
    else
    {
        ui->outputLabel->setText("Hash " + hash + " does not match. This mod is only compatible with Kenshi 1.0.51 x64 (Steam).");
        ui->nextButton->setEnabled(false);
    }
    ui->kenshiDirButton->setEnabled(true);
    ui->kenshiDirText->setEnabled(true);
}
