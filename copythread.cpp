#include "copythread.h"

#include <QWidget>

#include <fstream>

CopyThread::CopyThread(std::string source, std::string dest, QWidget *parent)
    : QThread(parent)
{
    this->sourcePath = source;
    this->destPath = dest;
}

void CopyThread::run() {
    std::ifstream source(sourcePath, std::ios::binary);
    std::ofstream dest(destPath, std::ios::binary);
    if(!source.is_open() || !dest.is_open())
    {
        QString errorMsg = tr("Error opening files...");
        errorMsg = errorMsg + QString::fromStdString(sourcePath) + (source.is_open() ? "1" : "0");
        errorMsg = errorMsg + QString::fromStdString(destPath) + (dest.is_open() ? "1" : "0");
        emit resultError(errorMsg);
    }
    else
    {
        dest << source.rdbuf();
        source.close();
        dest.close();
        emit resultSuccess();
    }
}
