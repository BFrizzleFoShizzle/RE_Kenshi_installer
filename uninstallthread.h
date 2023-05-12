#ifndef UNINSTALLTHREAD_H
#define UNINSTALLTHREAD_H

#include "basethread.h"
#include "installoptions.h"

class UninstallWindow;

class UninstallThread : public BaseThread
{
	Q_OBJECT
public:
	UninstallThread(UninstallWindow *window, InstallOptions options);
	void run() override;
signals:
	void resultSuccess();
	void resultCancel(bool modIsDisabled);
	void resultError(bool modIsDisabled);
private:
	UninstallWindow *window;
	InstallOptions options;
};

#endif // UNINSTALLTHREAD_H
