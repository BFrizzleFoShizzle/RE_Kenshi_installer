#include "basethread.h"

#include "config.h"
#include "md5.h"

#include <QMessageBox>
#include <QApplication>
#include <QEventLoop>
#include <QFileInfo>
#include <QFile>
#include <QDir>

#include <fstream>

bool BaseThread::FileExists(QString path)
{
	QFileInfo check_file(path);
	return check_file.exists() && check_file.isFile();
}

bool BaseThread::HashSupported(std::string hash)
{
	// I regret this
	if(hash == moddedKenshiSteamHash)
		return true;

	std::map<QString, QString> supportedVersions = GetSupportedVersions();

	for(auto version : supportedVersions)
	{
		if(version.second == QString(hash.c_str()))
			return true;
	}

	return false;
}

bool BaseThread::HashIsModded(std::string hash)
{
	return hash == moddedKenshiSteamHash;
}

// returns true if retrying
bool BaseThread::ShowFileLockedError(QString filename)
{
	QMessageBox fileIsLockedMsgBox;
	fileIsLockedMsgBox.setIcon(QMessageBox::Warning);
	fileIsLockedMsgBox.setWindowTitle(tr("File open error"));
	fileIsLockedMsgBox.setText(tr("The file ") + "\"" + filename + "\"" + tr(" is open in another process.")
										+ tr("\nPlease close any other processes before continuing."));
	fileIsLockedMsgBox.setStandardButtons(QMessageBox::Ok);
	fileIsLockedMsgBox.addButton(QMessageBox::Cancel);
	fileIsLockedMsgBox.setDefaultButton(QMessageBox::Ok);
	fileIsLockedMsgBox.moveToThread(QApplication::instance()->thread());

	QEventLoop loop;
	connect(this, &BaseThread::messageBoxFinished, &loop, &QEventLoop::quit );
	emit showMessageBox(&fileIsLockedMsgBox);
	loop.exec();

	return fileIsLockedMsgBox.result() == QMessageBox::Ok;
}

// throws std::exception
std::string BaseThread::GetHash(std::ifstream &file)
{
	char hash[33] = {0};
	if(file.is_open())
	{
		size_t size = file.tellg();
		std::vector<char> fileBytes(size);
		file.seekg(0);
		file.read(&fileBytes[0], fileBytes.size());
		md5::md5_t hasher(&fileBytes[0], fileBytes.size());
		hasher.get_string(hash);
	}
	else
	{
		throw std::exception("File to hash isn't open");
	}
	return hash;
}

// Exceptions: SourceFileMissingException, FileOpenFailedException, CancelException
// TODO replace with QFile::copy ?
void BaseThread::CopyFile(QString destPath, QString sourcePath)
{
	if(!QFile::exists(sourcePath))
		throw SourceFileMissingException();

	// QFile.rename() is unsafe to use if crossing a partition, so we just do everything manually
	// this works if the file already exists
	std::ifstream source(sourcePath.toStdString(), std::ios::binary);
	std::ofstream dest(destPath.toStdString(), std::ios::binary);
	while(!source.is_open() || !dest.is_open())
	{
		if(!source.is_open() && IsFileLocked(sourcePath.toStdString()))
		{
			if(!ShowFileLockedError(sourcePath))
				throw CancelException();

			source.open(sourcePath.toStdString(), std::ios::binary);
		}
		else if (!dest.is_open() && IsFileLocked(destPath.toStdString()))
		{
			if(!ShowFileLockedError(destPath))
				throw CancelException();

			dest.open(destPath.toStdString(), std::ios::binary);
		}
		else
		{
			// error opening + we don't know why - pass it along so we can emit an error
			break;
		}
	}

	// handle open error
	if(!source.is_open() || !dest.is_open())
	{
		QString errorMsg = "Error opening files...";
		errorMsg = errorMsg + "\n" + sourcePath + (source.is_open() ? "1" : "0");
		errorMsg = errorMsg + "\n" + destPath + (dest.is_open() ? "1" : "0");
		emit log(errorMsg);
		throw FileOpenFailedException();
		return;
	}

	// files are good, do copy
	dest << source.rdbuf();
	source.close();
	dest.close();
}

// adapted from https://stackoverflow.com/questions/2536524/copy-directory-using-qt
// Exceptions: SourceFileMissingException, FileOpenFailedException, CancelException
void BaseThread::CopyFolder(QString destPath, QString sourcePath)
{
	QDir sourceDir(sourcePath);
	if (!sourceDir.exists())
		throw SourceFileMissingException();

	foreach (QString subdir, sourceDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		QString destSubdirPath = destPath + QDir::separator() + subdir;
		sourceDir.mkpath(destSubdirPath);
		CopyFolder(destSubdirPath, sourcePath + QDir::separator() + subdir);
	}

	foreach (QString file, sourceDir.entryList(QDir::Files)) {
		CopyFile(destPath + QDir::separator() + file, sourcePath + QDir::separator() + file);
	}
}

// Exceptions: SourceFileMissingException, FileInUseException, DeleteFailException
void BaseThread::DeleteFile(QString path)
{
	if(!QFile::exists(path))
		throw SourceFileMissingException();

	while(IsFileLocked(path.toStdString()))
	{
		if(!ShowFileLockedError(path))
			throw FileInUseException();
	}

	// delete source on success
	// TODO move into loop to remove TOCTTOU?
	if(!QFile::remove(path))
		throw DeleteFailException();
}

// stolen from https://stackoverflow.com/questions/27758573/deleting-a-folder-and-all-its-contents-with-qt
bool BaseThread::DeleteFolder(const QString & dirName)
{
	bool result = true;
	QDir dir(dirName);

	if (dir.exists(dirName)) {
		Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
			if (info.isDir()) {
				result = DeleteFolder(info.absoluteFilePath());
			} else {
				result = QFile::remove(info.absoluteFilePath());
			}

			if (!result) {
				return result;
			}
		}
		result = dir.rmdir(dirName);
	}
	return result;
}

// Exceptions: SourceFileMissingException, FileOpenFailedException, CancelException, FileInUseException, DeleteFailException
void BaseThread::MoveFile(QString destPath, QString sourcePath)
{
	// copy
	CopyFile(destPath, sourcePath);

	// delete source on success
	DeleteFile(sourcePath);
}

BaseThread::BaseThread()
{

}
