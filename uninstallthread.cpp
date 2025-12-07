#include "uninstallthread.h"

#include "uninstallwindow.h"


#include <QFileInfo>
#include <QMessageBox>
#include <QEventLoop>
#include <QApplication>
#include <QDir>
#include <QStandardPaths>

#include <fstream>

enum UninstallStep
{
	CHECK_EXE_HASH,
	RESTORE_CONFIG,
	DELETE_MAIN_DLL,
	DELETE_SECONDARY_DLL,
	DELTETE_KENSHILIB,
	DELETE_DATA,
	DELETE_HEIGHTMAP,
	DELETE_SHORTCUTS,
	DELETE_EXECUTABLE,
	DONE
};

static int GetUninstallPercent(UninstallStep step)
{
	return (100 * step) / UninstallStep::DONE;
}

UninstallThread::UninstallThread(UninstallWindow *window, InstallOptions options)
	: window(window), options(options)
{

}

void UninstallThread::run()
{
	emit log("Starting uninstall...");
	try
	{
		connect(window, &UninstallWindow::messageBoxFinished, this, &UninstallThread::messageBoxFinished);

		/// double-check hash
		statusUpdate(tr("Double-checking hash..."));
		QString exePath = options.kenshiExePath;
		std::ifstream exeFile(exePath.toStdString(), std::ios::ate | std::ios::binary);
		std::string exeHash;
		if(exeFile.is_open())
		{
			exeHash = GetHash(exeFile);
			// continue
		}
		else
		{
			statusUpdate(tr("Kenshi executable could not be opened"));
			emit resultError(false);
			return;
		}

		/// make sure Kenshi isn't running
		QFileInfo executableInfo(options.kenshiExePath);
		while(IsProcessRunning(executableInfo.fileName().toStdWString()))
		{
			QMessageBox kenshiIsRunningMsgbox;
			kenshiIsRunningMsgbox.setIcon(QMessageBox::Warning);
			kenshiIsRunningMsgbox.setWindowTitle(tr("Please close Kenshi"));
			kenshiIsRunningMsgbox.setText(QString(tr("Kenshi is running - please close Kenshi\n") + executableInfo.fileName()));
			kenshiIsRunningMsgbox.setStandardButtons(QMessageBox::Ok);
			kenshiIsRunningMsgbox.addButton(QMessageBox::Cancel);
			kenshiIsRunningMsgbox.setDefaultButton(QMessageBox::Ok);
			kenshiIsRunningMsgbox.moveToThread(QApplication::instance()->thread());
			QEventLoop loop;
			connect(window, &UninstallWindow::messageBoxFinished, &loop, &QEventLoop::quit );
			emit showMessageBox(&kenshiIsRunningMsgbox);
			loop.exec();
			if(kenshiIsRunningMsgbox.result() == QMessageBox::Cancel)
			{
				emit resultCancel(false);
				return;
			}
		}

		/// final hash checks + revert config
		QString kenshiDir = options.GetKenshiInstallDir();
		if(HashIsModded(exeHash))
		{
			// it's unlikely anyone will ever see this error message, but put it here anyway
			statusUpdate(tr("Updating from RE_Kenshi versions before 0.2.0 is no longer supported."
							"\nplease uninstall the previous RE_Kenshi using an installer from before 0.2.8, and try installing again."));
			emit resultError(false);
			return;
		}
		else if(HashSupported(exeHash))
		{
			statusUpdate(tr("EXE hash matches vanilla kenshi. Reverting kenshi plugin config..."));
			QString pluginFilePath = kenshiDir + "Plugins_x64.cfg";
			try
			{
				if(FileExists(pluginFilePath))
				{
					QFile pluginConfigFile(pluginFilePath);
					pluginConfigFile.open(QFile::ReadOnly);
					QByteArray fileBytes = pluginConfigFile.readAll();
					pluginConfigFile.close();
					const char* removeString = "\nPlugin=RE_Kenshi\n";
					int removeOffset = fileBytes.indexOf(removeString);
					if(removeOffset == -1)
					{
						emit log("RE_Kenshi already disabled in plugins");
					}
					else
					{
						fileBytes.remove(removeOffset, strlen(removeString));
						QFile outFile(pluginFilePath);
						outFile.open(QFile::OpenModeFlag::WriteOnly);
						outFile.write(fileBytes);
						outFile.close();
						emit log("Plugin config update success");
					}
				}
				else
				{
					statusUpdate(tr("Could not find Kenshi plugin config at ") + pluginFilePath);
					emit resultError(true);
					return;
				}
			}
			catch(std::exception e)
			{
				emit log(QString("Error updating plugin config ") + e.what());
				statusUpdate(tr("Error updating plugin config file: ") +  e.what());

				emit resultError(true);
				return;
			}
		}
		else
		{
			// TODO this is most likely due to an update, should be possible to uninstall
			// even if the version isn't identified
			statusUpdate(tr("Hash doesn't match! This shouldn't be possible! No files changed, aborted. Mod not installed. It is now safe to close this window."));
			progressUpdate(GetUninstallPercent(DONE));
			reportBug(CHECK_EXE_HASH, "Hash doesn't match!");
			emit resultError(false);
			return;
		}

		/// Remove RE_Kenshi DLL
		QString reKenshiDllPath = kenshiDir + "RE_Kenshi.dll";
		if(FileExists(reKenshiDllPath))
		{
			statusUpdate(tr("Old files restored. Removing RE_Kenshi DLL..."));
			try
			{
				DeleteFile(reKenshiDllPath);
				emit log("RE_Kenshi DLL deleted successfully");
			}
			// TODO handle cancel?
			catch(std::exception e)
			{
				// soft error - continue
				emit log(QString("RE_Kenshi DLL exists but could not be deleted ") + e.what());
			}
		}
		else
		{
			// soft error - continue
			emit log(QString("RE_Kenshi DLL doesn't exist at ") + reKenshiDllPath);
		}
		progressUpdate(GetUninstallPercent(DELETE_MAIN_DLL));

		/// Remove CompressTools DLL
		QString compressToolsDllPath = kenshiDir + "CompressToolsLib.dll";
		if(FileExists(compressToolsDllPath))
		{
			statusUpdate(tr("Old files restored. Removing CompressTools DLL..."));
			try
			{
				DeleteFile(compressToolsDllPath);
				emit log("CompressTools DLL deleted successfully");
			}
			catch(std::exception e)
			{
				// soft error - continue
				emit log(QString("CompressTools DLL exists but could not be deleted ") + e.what());
			}
		}
		else
		{
			// soft error - continue
			emit log(QString("CompressTools DLL doesn't exist at ") + compressToolsDllPath);
		}
		progressUpdate(GetUninstallPercent(DELETE_SECONDARY_DLL));

		/// Remove KenshiLib DLL
		QString kenshiLibDllPath = kenshiDir + "KenshiLib.dll";
		if(FileExists(kenshiLibDllPath))
		{
			statusUpdate(tr("Old files restored. Removing KenshiLib DLL..."));
			try
			{
				DeleteFile(kenshiLibDllPath);
				emit log("KenshiLib DLL deleted successfully");
			}
			catch(std::exception e)
			{
				// soft error - continue
				emit log(QString("KenshiLib DLL exists but could not be deleted ") + e.what());
			}
		}
		else
		{
			// soft error - continue
			emit log(QString("KenshiLib DLL doesn't exist at ") + compressToolsDllPath);
		}
		progressUpdate(GetUninstallPercent(DELTETE_KENSHILIB));

		/// Remove data
		/// TODO delete the rest of ./RE_Kenshi/ ?
		QString tutorialImagePath = kenshiDir + "RE_Kenshi/game_speed_tutorial.png";
		if(FileExists(tutorialImagePath))
		{
			statusUpdate(tr("Old files restored. Removing RE_Kenshi data..."));
			try
			{
				DeleteFile(tutorialImagePath);
				emit log("RE_Kenshi data deleted successfully");
			}
			catch(std::exception e)
			{
				// soft error - continue
				emit log(QString("Tutorial image exists but could not be deleted ") + e.what());
			}
		}
		else
		{
			emit log(QString("Tutorial image doesn't exist at ") + tutorialImagePath);
		}

		QString rvasPath = kenshiDir + "RE_Kenshi/RVAs";
		if(QDir(rvasPath).exists())
		{
			statusUpdate(tr("Old files restored. Removing RE_Kenshi data (RVAs)..."));
			try
			{
				DeleteFolder(rvasPath);
				emit log("RE_Kenshi RVAs data deleted successfully");
			}
			catch(std::exception e)
			{
				// soft error - continue
				emit log(QString("RVAs exists but could not be deleted ") + e.what());
			}
		}
		else
		{
			emit log(QString("RVAs doesn't exist at ") + rvasPath);
		}

		QString localePath = kenshiDir + "RE_Kenshi/locale";
		if(QDir(localePath).exists())
		{
			statusUpdate(tr("Old files restored. Removing RE_Kenshi data (locale)..."));
			try
			{
				DeleteFolder(localePath);
				emit log("RE_Kenshi locale data deleted successfully");
			}
			catch(std::exception e)
			{
				// soft error - continue
				emit log(QString("Locale data exists but could not be deleted ") + e.what());
			}
		}
		else
		{
			emit log(QString("Locales doesn't exist at ") + localePath);
		}
		progressUpdate(GetUninstallPercent(DELETE_DATA));

		/// Remove compressed heightmap
		QString heightmapPath = kenshiDir + "data/newland/land/fullmap.cif";
		if(FileExists(heightmapPath))
		{
			statusUpdate(tr("Old files restored. Removing compressed heightmap..."));
			try
			{
				DeleteFile(heightmapPath);
				emit log("compressed heightmap deleted successfully");
			}
			catch(std::exception e)
			{
				// soft error - continue
				emit log(QString("Compressed heightmap exists but could not be deleted ") + e.what());
			}
		}
		else
		{
			// soft error - continue
			emit log(QString("Compressed heightmap doesn't exist at ") + heightmapPath);
		}
		progressUpdate(GetUninstallPercent(DELETE_HEIGHTMAP));

		QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/RE_Kenshi.lnk";
		if(FileExists(desktopPath))
		{
			statusUpdate(tr("Removing shortcuts"));
			try
			{
				DeleteFile(desktopPath);
				emit log("Desktop shortcut deleted successfully");
			}
			catch(std::exception e)
			{
				// soft error - continue
				emit log(QString("Desktop shortcut exists but could not be deleted ") + e.what());
			}
		}
		else
		{
			// soft error - continue
			emit log(QString("Desktop shortcut doesn't exist at ") + desktopPath);
		}

		QString startPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + "/RE_Kenshi.lnk";
		if(FileExists(startPath))
		{
			statusUpdate(tr("Removing shortcuts"));
			try
			{
				DeleteFile(startPath);
				emit log("Start shortcut deleted successfully");
			}
			catch(std::exception e)
			{
				// soft error - continue
				emit log(QString("Start shortcut exists but could not be deleted ") + e.what());
			}
		}
		else
		{
			// soft error - continue
			emit log(QString("Start shortcut doesn't exist at ") + startPath);
		}
		progressUpdate(GetUninstallPercent(DELETE_SHORTCUTS));

		/// Remove executable
		QString executablePath = kenshiDir + "RE_Kenshi.exe";
		if(FileExists(executablePath))
		{
			statusUpdate(tr("Old files restored. Removing executable..."));
			try
			{
				DeleteFile(executablePath);
				emit log("executable deleted successfully");
			}
			catch(std::exception e)
			{
				// soft error - continue
				emit log(QString("executable exists but could not be deleted ") + e.what());
			}
		}
		else
		{
			// soft error - continue
			emit log(QString("executable doesn't exist at ") + executablePath);
		}
		progressUpdate(GetUninstallPercent(DELETE_EXECUTABLE));

		// success
		progressUpdate(GetUninstallPercent(DONE));
		emit log("Uninstall success");
		// TODO report error'd files
		emit resultSuccess();
		return;
	}
	catch(std::exception e)
	{
		statusUpdate(tr("UNCAUGHT ERROR?!? Sorry, I probably broke your kenshi install. Rename \"kenshi_x64_vanilla.exe\" to \"kenshi_x64.exe\" and \"Plugins_x64_vanilla.cfg\" to \"Plugins_x64.cfg\" to fix whatever I've done... :("));
		reportBug(DONE, QString("UNCAUGHT ERROR: this should never happen ") + e.what());
		// TODO ???
		//emit resultError(true);
		return;
	}
	statusUpdate(tr("UNCAUGHT ERROR?!? Sorry, I probably broke your kenshi install. Rename \"kenshi_x64_vanilla.exe\" to \"kenshi_x64.exe\" and \"Plugins_x64_vanilla.cfg\" to \"Plugins_x64.cfg\" to fix whatever I've done... :("));
	reportBug(DONE, "Ran past end of uninstall code");
	// TODO ???
	//emit resultError(true);
	return;
}
