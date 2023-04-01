#include "copythread.h"

#include <QWidget>
#include <QString>
#include <QMessageBox>


#include <fstream>

#include "process.h"

CopyThread::CopyThread(std::ifstream* source, std::ofstream* dest, std::string sourceName, std::string destName, QWidget *parent)
    : QThread(parent)
{
	this->source = source;
	this->dest = dest;
	this->sourceName = sourceName;
	this->destName = destName;
}

// returns true if retrying
static bool ShowFileLockedError(QString filename)
{

	QMessageBox kenshiIsRunningMsgbox;
	kenshiIsRunningMsgbox.setIcon(QMessageBox::Warning);
	kenshiIsRunningMsgbox.setWindowTitle(QObject::tr("File open error"));
	kenshiIsRunningMsgbox.setText(QObject::tr("The file ") + "\"" + filename + "\"" + QObject::tr(" is open in another process.")
										+ QObject::tr("\nPlease close any other processes before continuing."));
	kenshiIsRunningMsgbox.setStandardButtons(QMessageBox::Ok);
	kenshiIsRunningMsgbox.addButton(QMessageBox::Cancel);
	kenshiIsRunningMsgbox.setDefaultButton(QMessageBox::Ok);

	return kenshiIsRunningMsgbox.exec() == QMessageBox::Ok;
}

CopyThread* CopyThread::CreateCopyThread(std::string sourcePath, std::string destPath, QWidget *parent)
{
	std::ifstream* source = new std::ifstream(sourcePath, std::ios::binary);
	std::ofstream* dest = new std::ofstream(destPath, std::ios::binary);
	while(!source->is_open() || !dest->is_open())
	{
		QString errorMsg = tr("Error opening files...");
		errorMsg = errorMsg + QString::fromStdString(sourcePath) + (source->is_open() ? "1" : "0");
		errorMsg = errorMsg + QString::fromStdString(destPath) + (dest->is_open() ? "1" : "0");

		if(!source->is_open() && IsFileLocked(sourcePath))
		{
			if(!ShowFileLockedError(QString::fromStdString(sourcePath)))
				return new CopyThread(nullptr, nullptr, sourcePath, destPath, parent);

			source->open(sourcePath, std::ios::binary);
		}
		else if (!dest->is_open() && IsFileLocked(destPath))
		{
			if(!ShowFileLockedError(QString::fromStdString(destPath)))
				return new CopyThread(nullptr, nullptr, sourcePath, destPath, parent);

			dest->open(destPath, std::ios::binary);
		}
		else
		{
			// error opening + we don't know why - pass it along to the thread so we can emit an error
			break;
		}
	}
	return new CopyThread(source, dest, sourcePath, destPath, parent);
}

void CopyThread::run()
{
	// handle cancel
	if(source == nullptr || dest == nullptr)
	{
		emit resultCancel();
		return;
	}

	// handle open error
	if(!source->is_open() || !dest->is_open())
	{
		QString errorMsg = tr("Error opening files...");
		errorMsg = errorMsg + QString::fromStdString(sourceName) + (source->is_open() ? "1" : "0");
		errorMsg = errorMsg + QString::fromStdString(destName) + (dest->is_open() ? "1" : "0");
		emit resultError(errorMsg);
		return;
	}

	// files are good, do copy
	*dest << source->rdbuf();
	source->close();
	dest->close();
	delete source;
	delete dest;
	emit resultSuccess();
}
