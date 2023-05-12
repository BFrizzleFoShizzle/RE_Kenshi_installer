#ifndef BASETHREAD_H
#define BASETHREAD_H

#include <QThread>

class QMessageBox;

class BaseThread : public QThread
{
	Q_OBJECT
protected:
	BaseThread();

	class FileInUseException : public std::exception
	{
	public:
		FileInUseException()
			: std::exception("FileInUse")
		{};
	};
	class CancelException : public std::exception
	{
	public:
		CancelException()
			: std::exception("Cancel")
		{};
	};
	class FileOpenFailedException : public std::exception
	{
	public:
		FileOpenFailedException()
			: std::exception("FileOpenFailed")
		{};
	};
	class DeleteFailException : public std::exception
	{
	public:
		DeleteFailException()
			: std::exception("DeleteFail")
		{};
	};
	class SourceFileMissingException : public std::exception
	{
	public:
		SourceFileMissingException()
			: std::exception("SourceFileMissing")
		{};
	};

	bool FileExists(QString path);
	bool HashIsModded(std::string hash);
	bool HashSupported(std::string hash);
	// Exceptions: std::exception
	std::string GetHash(std::ifstream &file);
	// Exceptions: SourceFileMissingException, FileOpenFailedException, CancelException
	void CopyFile(QString destPath, QString sourcePath);
	// Exceptions: SourceFileMissingException, FileOpenFailedException, CancelException
	void CopyFolder(QString destPath, QString sourcePath);
	// Exceptions: SourceFileMissingException, FileOpenFailedException, CancelException, FileInUseException, DeleteFailException
	void MoveFile(QString destPath, QString sourcePath);
	// Exceptions: SourceFileMissingException, FileInUseException, DeleteFailException
	void DeleteFile(QString path);
	bool ShowFileLockedError(QString filename);
signals:
	void showMessageBox(QMessageBox *s);
	void reportBug(int step, QString info);
	void progressUpdate(int percent);
	void statusUpdate(QString text);
	void log(QString text);
	void messageBoxFinished();
};

#endif // BASETHREAD_H
