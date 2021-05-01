#ifndef HASHTHREAD_H
#define HASHTHREAD_H

#include <QThread>

const std::string moddedKenshiSteamHash = "a5f78908f3f26591a6a3717bfbfafbca";
const std::string vanillaKenshiSteamHash = "57679de0ae258ead45a96949974517e2";
const std::string vanillaKenshiGOGHash = "525261fca4d339da67999c114118c6c6";

class HashThread : public QThread
{
    Q_OBJECT
public:
        HashThread(std::string file, QWidget *parent = nullptr);
        void run() override;
    signals:
        void resultSuccess(const QString &s);
        void resultError(const QString &s);
private:
    std::string filePath;
};
#endif // HASHTHREAD_H
