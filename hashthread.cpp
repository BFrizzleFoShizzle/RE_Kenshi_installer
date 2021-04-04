#include "hashthread.h"

#include <QWidget>

#include <fstream>

#include "md5.h"

HashThread::HashThread(std::string file, QWidget *parent)
    : QThread(parent)
{
    this->filePath = file;
}
void HashThread::run() {
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if(file.is_open())
    {
        size_t size = file.tellg();
        std::vector<char> fileBytes(size);
        file.seekg(0);
        file.read(&fileBytes[0], fileBytes.size());
        md5::md5_t hasher(&fileBytes[0], fileBytes.size());
        char hashStr[33];
        hasher.get_string(hashStr);
        emit resultSuccess(hashStr);
    }
    else
    {
        emit resultError("File could not be opened");
    }
}
