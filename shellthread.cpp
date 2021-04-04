#include "shellthread.h"

#include <QWidget>

ShellThread::ShellThread(std::string command, QWidget *parent)
    : QThread(parent)
{
    this->command = command;
}

void ShellThread::run()
{
    int result = system(command.c_str());
    if (result == 0)
    {
        emit resultSuccess();
    }
    else
    {
        emit resultError(result);
    }
}
