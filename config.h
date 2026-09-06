#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <map>

const std::string moddedKenshiSteamHash = "a5f78908f3f26591a6a3717bfbfafbca";

std::map<QString, QString> GetSupportedVersions();

bool HashRequiresDowngrade(std::string hash);

#endif // CONFIG_H
