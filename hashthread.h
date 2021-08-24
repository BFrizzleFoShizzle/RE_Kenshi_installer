#ifndef HASHTHREAD_H
#define HASHTHREAD_H

#include <QThread>

class HashThread : public QThread
{
    Q_OBJECT
public:
    static bool HashSupported(std::string hash);
    static bool HashIsModded(std::string hash);
    HashThread(std::string file, QWidget *parent = nullptr);
    void run() override;
signals:
    void resultSuccess(const QString &s);
    void resultError(const QString &s);
private:
    std::string filePath;
};
#endif // HASHTHREAD_H
