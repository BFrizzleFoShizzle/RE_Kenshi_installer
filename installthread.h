#ifndef INSTALLTHREAD_H
#define INSTALLTHREAD_H

#include "basethread.h"
#include "installoptions.h"

class InstallWindow;

enum InstallStep;

class InstallThread : public BaseThread
{
	Q_OBJECT
public:
	InstallThread(InstallWindow *window, InstallOptions options);
	void run() override;
signals:
	void resultSuccess();
	void resultCancel(bool requireUninstall);
	void resultError(bool requireUninstall);
private:
	bool CopyChecked(QString dest, QString src, InstallStep step, QString fileDescription);
	InstallWindow *window;
	InstallOptions options;
};

#endif // INSTALLTHREAD_H
