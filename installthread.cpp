#include "installthread.h"

#include "installwindow.h"

#include "config.h"

#include <QFileInfo>
#include <QDir>
#include <QEventLoop>
#include <QMessageBox>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>

#include <fstream>

enum InstallStep
{
	HASH_CHECK,
	MAIN_COPY,
	SECONDARY_COPY,
	KENSHILIB_COPY,
	DATA_COPY,
	MOD_SETTINGS_UPDATE,
	COMPRESS,
	// hack to make compression take most of the bar
	COMPRESS_2,
	COMPRESS_3,
	COMPRESS_4,
	COMPRESS_5,
	GAME_DOWNGRADE,
	SHORTCUTS, // only needed if version != 1.0.65
	CONFIG_APPEND,
	DONE
};

// Adapted from https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
static std::string shellExec(std::string cmd) {
	char buffer[128];
	std::string result = "";
	FILE* pipe = _popen(cmd.c_str(), "r");
	if (!pipe)
		return "POPEN_ERROR";
	try {
		while (fgets(buffer, sizeof buffer, pipe) != NULL) {
			result += buffer;
		}
	} catch (...) {
		_pclose(pipe);
		throw;
	}
	_pclose(pipe);
	return result;
}

static int GetInstallPercent(InstallStep step)
{
	return (100 * step) / InstallStep::DONE;
}

InstallThread::InstallThread(InstallWindow *window, InstallOptions options)
	: window(window), options(options)
{

}

// boilerplate code for common file copies
// emits all required events + returns whether successful
bool InstallThread::CopyChecked(QString destPath, QString srcPath, InstallStep step, QString fileDescription)
{
	emit log("Copying " + fileDescription);
	if(FileExists(srcPath))
	{
		try
		{
			CopyFile(destPath, srcPath);
			emit log(fileDescription + " copied successfully");
			return true;
		}
		catch(CancelException)
		{
			log("cancel @ " + fileDescription + " copy");
			emit resultCancel(true);
			return false;
		}
		catch(std::exception e)
		{
			// "Error copying [file name] [error message]"
			statusUpdate(tr("Error copying ") + fileDescription + " " + e.what());
			reportBug(step, "Error copying " + fileDescription);
			emit resultError(true);
			return false;
		}
	}
	else
	{
		// file doesn't exist error - "[file] doesn't exist at [path]"
		statusUpdate(fileDescription + tr(" doesn't exist at ") + srcPath);
		emit resultError(true);
		return false;
	}
}

void InstallThread::run()
{
	emit log("Starting install...");
	try
	{
		connect(window, &InstallWindow::messageBoxFinished, this, &InstallThread::messageBoxFinished);

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
			connect(window, &InstallWindow::messageBoxFinished, &loop, &QEventLoop::quit );
			emit showMessageBox(&kenshiIsRunningMsgbox);
			loop.exec();
			if(kenshiIsRunningMsgbox.result() == QMessageBox::Cancel)
			{
				resultCancel(false);
				return;
			}
		}

		/// final hash checks + backup config
		QString kenshiDir = options.GetKenshiInstallDir();
		if(HashIsModded(exeHash))
		{
			// it's unlikely anyone will ever see this error message, but put it here anyway
			statusUpdate(tr("Updating from RE_Kenshi versions before 0.2.0 is no longer supported."
							"\nplease uninstall the previous RE_Kenshi using an installer from before 0.2.8, and try installing again."));
			emit resultError(false);
			return;
		}
		else if(!HashSupported(exeHash))
		{
			// if we reach here, the hash must have changed between the first window and this window
			statusUpdate(tr("Hash doesn't match! This shouldn't be possible! No files changed, aborted. Mod not installed. It is now safe to close this window."));
			reportBug(HASH_CHECK, "Hash doesn't match!");
			emit resultError(false);
			return;
		}
		progressUpdate(GetInstallPercent(HASH_CHECK));

		/// copy RE_Kenshi DLL
		statusUpdate(tr("Copying mod files..."));
		QString reKenshiDllDestPath = kenshiDir + "RE_Kenshi.dll";
		QString reKenshiDllSrcPath = "install/RE_Kenshi.dll";
		if(!CopyChecked(reKenshiDllDestPath, reKenshiDllSrcPath, MAIN_COPY, "RE_Kenshi DLL"))
			return;
		progressUpdate(GetInstallPercent(MAIN_COPY));

		/// copy CompressTools DLL
		QString compressToolsDllDestPath = kenshiDir + "CompressToolsLib.dll";
		QString compressToolsSrcPath = "install/CompressToolsLib.dll";
		if(!CopyChecked(compressToolsDllDestPath, compressToolsSrcPath, SECONDARY_COPY, "CompressTools DLL"))
			return;
		progressUpdate(GetInstallPercent(SECONDARY_COPY));

		// copy KenshiLib
		QString kenshiLibDllDestPath = kenshiDir + "KenshiLib.dll";
		QString kenshiLibSrcPath = "install/KenshiLib.dll";
		if(!CopyChecked(kenshiLibDllDestPath, kenshiLibSrcPath, KENSHILIB_COPY, "KenshiLib DLL"))
			return;
		progressUpdate(GetInstallPercent(KENSHILIB_COPY));

		/// Copy data
		// create data directory
		QString dataDirectory = kenshiDir + "RE_Kenshi/";
		if(!QDir().mkpath(dataDirectory))
		{
			statusUpdate(tr("Could not create folder ") + dataDirectory);
			emit resultError(true);
			return;
		}

		// copy data files
		if(QDir("install/RE_Kenshi").exists())
		{
			try
			{
				CopyFolder(dataDirectory, "install/RE_Kenshi");
				emit log("Translation files copied successfully");
			}
			catch(CancelException)
			{
				emit resultCancel(true);
				return;
			}
			catch(std::exception e)
			{
				statusUpdate(tr("Error copying data files") + e.what());
				reportBug(DATA_COPY, "Error copying data files");
				emit resultError(true);
				return;
			}
		}
		else
		{
			statusUpdate(tr("Data files missing at ") + "./install/RE_Kenshi");
			emit resultError(true);
			return;
		}

		/// Config update
		statusUpdate(tr("Updating mod config..."));
		// clear shader cache
		if(options.clearShaderCache)
		{
			try
			{
				QFileInfo checkFile(dataDirectory + "shader_cache.sc");
				if(checkFile.exists() && checkFile.isFile())
				{
					QFile file (checkFile.absoluteFilePath());
					file.remove();
					emit log("Shader cache cleared successfully");
				}
			}
			catch (const std::exception& e)
			{
				// doesn't really matter if this fails, it's optional anyway
				emit log("Exception while clearing shader cache, continuing...");
			}
		}
		// update settings
		try
		{
			QFile modConfigFile(kenshiDir + "RE_Kenshi.ini");
			modConfigFile.open(QFile::ReadOnly);
			QJsonDocument jsonDoc = QJsonDocument().fromJson(modConfigFile.readAll());
			modConfigFile.close();
			QJsonObject jsonObj = jsonDoc.object();
			jsonObj.insert("CheckUpdates", options.checkUpdates);
			// clear skipped version
			if(options.clearSkippedVersions)
				jsonObj.insert("SkippedVersion", "");
			jsonDoc.setObject(jsonObj);
			modConfigFile.open(QFile::WriteOnly);
			modConfigFile.write(jsonDoc.toJson());
			modConfigFile.close();
			emit log("RE_Kenshi.ini updated successfully");
		}
		catch (const std::exception& e)
		{
			// doesn't really matter if this fails, it's optional anyway
			emit log("Exception while updating RE_Kenshi.ini, continuing...");
		}
		progressUpdate(GetInstallPercent(MOD_SETTINGS_UPDATE));

		/// Compress heightmap
		if(options.compressHeightmap)
		{
			statusUpdate(tr("Compressing heightmap, this may take a minute or two..."));
			QString heightmapReadPath = kenshiDir + "data/newland/land/fullmap.tif";
			QString heightmapWritePath = kenshiDir + "data/newland/land/fullmap.cif";
			QString command = "tools\\CompressTools.exe \"" + heightmapReadPath + "\" \"" + heightmapWritePath + "\"";
			int result = system(command.toStdString().c_str());
			// doesn't really matter if this fails, it's optional anyway
			if(result != 0)
				emit log("Error: Heightmap compression returned " + QString::number(result));
		}
		progressUpdate(GetInstallPercent(COMPRESS_5));

		/// Downgrade game
		if(HashRequiresDowngrade(exeHash))
		{
			QString patchFile = QFileInfo(options.kenshiExePath).fileName() + ".patch";

			statusUpdate(tr("Downgrading executable..."));
			std::string command = (".\\tools\\courgette64.exe -apply \"" + options.kenshiExePath + "\" ./tools/" + patchFile + " \"" + kenshiDir + "/RE_Kenshi.exe\"").toStdString();
			emit log(QString::fromStdString(shellExec(command)));

			progressUpdate(GetInstallPercent(GAME_DOWNGRADE));

			statusUpdate(tr("Creating shortcuts..."));
			QFile newExecutable(kenshiDir + "/RE_Kenshi.exe");
			if(options.createDesktopShortcut)
			{
				QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
				newExecutable.link(desktopPath+"/RE_Kenshi.lnk");
			}
			if(options.createStartShortcut)
			{
				QString startPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
				newExecutable.link(startPath+"/RE_Kenshi.lnk");
			}
		}

		progressUpdate(GetInstallPercent(SHORTCUTS));

		/// Update plugin config
		statusUpdate(tr("Adding RE_Kenshi to plugin config file..."));
		QString pluginFilePath = kenshiDir + "Plugins_x64.cfg";
		try
		{
			if(FileExists(pluginFilePath))
			{
				QFile pluginConfigFile(pluginFilePath);
				pluginConfigFile.open(QFile::ReadOnly);
				QByteArray fileBytes = pluginConfigFile.readAll();
				pluginConfigFile.close();
				if(fileBytes.contains("\nPlugin=RE_Kenshi"))
				{
					emit log("RE_Kenshi already enabled in plugins");
				}
				else
				{
					QFile outFile(pluginFilePath);
					outFile.open(QFile::OpenModeFlag::Append);
					outFile.write("\nPlugin=RE_Kenshi\n");
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
		progressUpdate(GetInstallPercent(DONE));

		/// success
		emit log("Install success");
		// TODO report error'd files
		emit resultSuccess();
		return;
	}
	catch(std::exception e)
	{
		statusUpdate(tr("UNCAUGHT ERROR?!? Sorry, I probably broke your kenshi install. Please remove the line \"Plugin=RE_Kenshi\" from \"Plugins_x64.cfg\" to fix whatever I've done... :("));
		reportBug(DONE, QString("UNCAUGHT ERROR: this should never happen ") + e.what());
		// TODO ???
		//emit resultError(true);
		return;
	}
	statusUpdate(tr("UNCAUGHT ERROR?!? Sorry, I probably broke your kenshi install. Please remove the line \"Plugin=RE_Kenshi\" from \"Plugins_x64.cfg\" to fix whatever I've done... :("));
	reportBug(DONE, "Ran past end of install code");
	// TODO ???
	//emit resultError(true);
	return;
}
