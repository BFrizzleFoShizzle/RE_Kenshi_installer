#ifndef INSTALLOPTIONS_H
#define INSTALLOPTIONS_H

#include <QString>
#include <QStringList>

struct InstallOptions
{
	InstallOptions()
		: kenshiExePath(""),
		  compressHeightmap(false),
		  checkUpdates(false),
		  clearSkippedVersions(false),
		  clearShaderCache(false)
	{

	}
	QString GetKenshiInstallDir()
	{
		return kenshiExePath.toLower().split("kenshi_gog_x64.exe")[0].split("kenshi_x64.exe")[0];
	}
	QString kenshiExePath;
	bool compressHeightmap;
	bool checkUpdates;
	bool clearSkippedVersions;
	bool clearShaderCache;
};

#endif // INSTALLOPTIONS_H
