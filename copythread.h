#ifndef COPYTHREAD_H
#define COPYTHREAD_H

#include <QThread>

class CopyThread : public QThread
{
    Q_OBJECT
	public:
		void run() override;
		static CopyThread* CreateCopyThread(std::string source, std::string dest, QWidget *parent = nullptr);
    signals:
        void resultSuccess();
		void resultCancel();
        void resultError(const QString &s);
private:
	CopyThread(std::ifstream* source, std::ofstream* dest, std::string sourceName, std::string destName, QWidget *parent = nullptr);
	std::ifstream* source;
	std::ofstream* dest;
	// used for debugging
	std::string sourceName;
	std::string destName;
};

#endif // COPYTHREAD_H
