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

#endif // CONFIG_H
