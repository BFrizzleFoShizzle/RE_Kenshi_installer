#ifndef SHELLTHREAD_H
#define SHELLTHREAD_H


#include <QThread>

class ShellThread : public QThread
{
    Q_OBJECT
public:
        ShellThread(std::string command, QWidget *parent = nullptr);
        void run() override;
    signals:
        void resultSuccess();
        void resultError(const int &i);
private:
    std::string command;
};
#endif // SHELLTHREAD_H
