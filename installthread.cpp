#include "installthread.h"

#include "installwindow.h"

#include <QFileInfo>
#include <QDir>
#include <QEventLoop>
#include <QMessageBox>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>

#include <fstream>

enum InstallStep
{
	HASH_CHECK,
	BACKUP_COPY,
	MAIN_COPY,
	SECONDARY_COPY,
	TUT_IMAGE_COPY,
	TRANSLATION_COPY,
	MOD_SETTINGS_UPDATE,
	COMPRESS,
	// hack to make compression take most of the bar
	COMPRESS_2,
	COMPRESS_3,
	COMPRESS_4,
	COMPRESS_5,
	CONFIG_APPEND,
	DONE
};

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
		else if(HashSupported(exeHash))
		{
			statusUpdate(tr("Hash matches. Making kenshi plugin config backup..."));
			QString pluginsConfigPath = kenshiDir + "Plugins_x64.cfg";
			QString pluginsConfigBackupPath = kenshiDir + "Plugins_x64_vanilla.cfg";

			if(FileExists(pluginsConfigBackupPath))
			{
				// the uninstaller should usually prevent this from happening
				emit log("Plugin config already backed up, skipping...");
			}
			else
			{
				try
				{
					CopyFile(pluginsConfigBackupPath, pluginsConfigPath);
					emit log("Plugin config backup succeeded");
				}
				// cancelling at this point is fine
				catch(CancelException)
				{
					emit log("Plugin config backup cancelled");
					emit resultCancel(false);
					return;
				}
				// any other error needs to be reported
				catch(std::exception e)
				{
					statusUpdate(tr("Error backing up plugin config file: ") + e.what());
					reportBug(BACKUP_COPY, QString("Error: ") + e.what());
					emit resultError(false);
					return;
				}
			}
		}
		else
		{
			// if we reach here, the hash must have changed between the first window and this window
			statusUpdate(tr("Hash doesn't match! This shouldn't be possible! No files changed, aborted. Mod not installed. It is now safe to close this window."));
			reportBug(HASH_CHECK, "Hash doesn't match!");
			emit resultError(false);
			return;
		}
		progressUpdate(GetInstallPercent(BACKUP_COPY));

		/// copy RE_Kenshi DLL
		statusUpdate(tr("Copying mod files..."));
		QString reKenshiDllDestPath = kenshiDir + "RE_Kenshi.dll";
		QString reKenshiDllSrcPath = "tools/RE_Kenshi.dll";
		if(!CopyChecked(reKenshiDllDestPath, reKenshiDllSrcPath, MAIN_COPY, "RE_Kenshi DLL"))
			return;
		progressUpdate(GetInstallPercent(MAIN_COPY));

		/// copy CompressTools DLL
		QString compressToolsDllDestPath = kenshiDir + "CompressToolsLib.dll";
		QString compressToolsSrcPath = "tools/CompressToolsLib.dll";
		if(!CopyChecked(compressToolsDllDestPath, compressToolsSrcPath, SECONDARY_COPY, "CompressTools DLL"))
			return;
		progressUpdate(GetInstallPercent(SECONDARY_COPY));

		/// Copy data
		// create data directory
		QString dataDirectory = kenshiDir + "RE_Kenshi/";
		if(!QDir().mkpath(dataDirectory))
		{
			statusUpdate(tr("Could not create folder ") + dataDirectory);
			emit resultError(true);
			return;
		}
		// copy tutorial image
		QString tutImageDestPath = kenshiDir + "RE_Kenshi/game_speed_tutorial.png";
		QString tutImageSrcPath = "tools/game_speed_tutorial.png";
		// TODO translate name?
		if(!CopyChecked(tutImageDestPath, tutImageSrcPath, TUT_IMAGE_COPY, "game speed tutorial image"))
			return;
		progressUpdate(GetInstallPercent(TUT_IMAGE_COPY));

		// copy translation files
		if(QDir("tools/locale").exists())
		{
			try
			{
				CopyFolder(dataDirectory + "locale", "tools/locale");
				emit log("Translation files copied successfully");
			}
			catch(CancelException)
			{
				emit log("cancel @ game speed tut image copy");
				emit resultCancel(true);
				return;
			}
			catch(std::exception e)
			{
				statusUpdate(tr("Error copying data files") + e.what());
				reportBug(TRANSLATION_COPY, "Error copying translation files");
				emit resultError(true);
				return;
			}
		}
		else
		{
			statusUpdate(tr("Translation files missing at ") + "./tools/locale");
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
		statusUpdate(tr("UNCAUGHT ERROR?!? Sorry, I probably broke your kenshi install. Rename \"kenshi_x64_vanilla.exe\" to \"kenshi_x64.exe\" and \"Plugins_x64_vanilla.cfg\" to \"Plugins_x64.cfg\" to fix whatever I've done... :("));
		reportBug(DONE, QString("UNCAUGHT ERROR: this should never happen ") + e.what());
		// TODO ???
		//emit resultError(true);
		return;
	}
	statusUpdate(tr("UNCAUGHT ERROR?!? Sorry, I probably broke your kenshi install. Rename \"kenshi_x64_vanilla.exe\" to \"kenshi_x64.exe\" and \"Plugins_x64_vanilla.cfg\" to \"Plugins_x64.cfg\" to fix whatever I've done... :("));
	reportBug(DONE, "Ran past end of install code");
	// TODO ???
	//emit resultError(true);
	return;
}
