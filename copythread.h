#ifndef COPYTHREAD_H
#define COPYTHREAD_H

#include <QThread>

class CopyThread : public QThread
{
    Q_OBJECT
    public:
            CopyThread(std::string source, std::string dest, QWidget *parent = nullptr);
            void run() override;
    signals:
        void resultSuccess();
        void resultError(const QString &s);
private:
    std::string sourcePath;
    std::string destPath;
};

#endif // COPYTHREAD_H
