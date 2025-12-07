#ifndef CONFIG_H
#define CONFIG_H

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

const std::string moddedKenshiSteamHash = "a5f78908f3f26591a6a3717bfbfafbca";

static std::map<QString, QString> GetSupportedVersions()
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

static bool HashRequiresDowngrade(std::string hash)
{
	std::map<QString, QString> supportedVersions = GetSupportedVersions();

	for(auto version : supportedVersions)
	{
		if(version.second == QString(hash.c_str()))
			// downgrade if not 1.0.65
			return !version.first.endsWith("1.0.65");
	}

	return false;
}
#endif // CONFIG_H
