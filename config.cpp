#include "config.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

std::map<QString, QString> GetSupportedVersions()
{
	QFile installerConfigFile("config.json");
	installerConfigFile.open(QFile::ReadOnly);
	QJsonDocument jsonDoc = QJsonDocument().fromJson(installerConfigFile.readAll());
	installerConfigFile.close();
	QJsonObject jsonObj = jsonDoc.object();
	QJsonObject supportedVersionsObj = jsonObj.value("supportedVersions").toObject();

	std::map<QString, QString> supportedVersions;
	for(auto key : supportedVersionsObj.keys())
	{
		// force deep copy
		supportedVersions.emplace(key, supportedVersionsObj.value(key).toString());
	}

	return supportedVersions;
}

bool HashRequiresDowngrade(std::string hash)
{
	std::map<QString, QString> supportedVersions = GetSupportedVersions();

	for(auto version : supportedVersions)
	{
		if(version.second == QString(hash.c_str()))
			// downgrade if not 1.0.65
			return !version.first.endsWith("1.0.65");
	}

	// TODO error
	return false;
}

/*
static QString GetHashVersion(std::string hash)
{
	std::map<QString, QString> supportedVersions = GetSupportedVersions();

	for(auto version : supportedVersions)
	{
		if(version.second == QString(hash.c_str()))
			return version.first;
	}

	return QString("Unknown");
}
*/
