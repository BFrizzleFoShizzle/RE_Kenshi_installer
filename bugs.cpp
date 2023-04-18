#include "bugs.h"

#include "discord.h"

#include "md5.h"

#include <QMessageBox>
#include <QEventLoop>
#include <QUrl>
#include <QUrlQuery>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QObject>
#include <QCheckBox>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

std::string installerVersion = "0.2";

static std::string GetUUID()
{
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
	{
		// should be 37 characters + null
		char buffer[40];
		DWORD dwType, dwSize = sizeof(buffer);

		if (RegQueryValueExA(hKey, "MachineGuid", NULL, &dwType, reinterpret_cast<LPBYTE>(buffer), &dwSize) == ERROR_SUCCESS)
		{
			if (dwType == REG_SZ)
			{
				RegCloseKey(hKey);
				return std::string(buffer, dwSize);
			}
			else
			{
				//ErrorLog("UUID key has wrong type");
			}
		}
		else
		{
			//ErrorLog("Failed to query UUID key ");
		}

		RegCloseKey(hKey);
	}
	else
	{
		//ErrorLog("Failed to open UUID key");
	}
	return "";
}

static std::string GetUUIDHash()
{
	std::string uuid = GetUUID();

	if (uuid == "")
		return "No UUID";

	// calculate hash
	md5::md5_t hasher(uuid.c_str(), uuid.size());
	char hashStr[33];
	hasher.get_string(hashStr);

	// we don't need the full 32 chars to identify
	// 32 bits = 1% chance of collision at 10k bug reporters, plenty safe
	// 8 chars = 32 bits (1 byte = 2 chars)
	std::string finalHash = std::string(hashStr).substr(0, 8);
	transform(finalHash.begin(), finalHash.end(), finalHash.begin(), toupper);
	return finalHash;
}


void Bugs::ReportBug(std::string window, int step, std::string error)
{
	// get user consent
	QMessageBox consentBox;
	QCheckBox uuidCheckBox("Include UUID hash");
	QString hash = QString::fromStdString(GetUUIDHash());
	uuidCheckBox.setChecked(true);
	consentBox.setIcon(QMessageBox::Critical);
	consentBox.setWindowTitle(QObject::tr("Error"));
	consentBox.setText(QString(QObject::tr("RE_Kenshi's installer has encountered an error"))
					   + QObject::tr("\nWould you like to send an error report to the RE_Kenshi team?")
					   + QObject::tr("\nYour report will be sent to RE_Kenshi's developer (BFrizzleFoShizzle) with the following information:")
					   + "\n" + QObject::tr("\nInstaller version: ") + QString::fromStdString(installerVersion)
					   + QObject::tr("\nWindow: ") + QString::fromStdString(window) // the name of the window - e.g. InstallerWindow, UninstallerWindow
					   + QObject::tr("\nInstallation step: ") + QString::number(step)
					   + QObject::tr("\nUUID hash: ") + hash + QObject::tr(" (optional - allows the developer to know all your reports come from the same machine)")
					   + QObject::tr("\nError message: ")
					   + "\n" + QString::fromStdString(error) + "\n");
	consentBox.setCheckBox(&uuidCheckBox);
	consentBox.setStandardButtons(QMessageBox::Yes);
	consentBox.addButton(QMessageBox::No);
	consentBox.setDefaultButton(QMessageBox::Yes);

	if(consentBox.exec() != QMessageBox::Yes)
		return;

	// We have consent to upload error report

	// Create bug report message
	QString message = QString::fromStdString("Installer " + installerVersion + " error:");
	if (uuidCheckBox.isChecked())
		message += "\nUUID: " + hash;
	message += QString::fromStdString("\nWindow: " + window
		+ "\nStep: " + std::to_string(step) + "\nError message:");

	QUrl url = QUrl(QString::fromStdString(installerBugDiscordWebHookURL));
	QNetworkAccessManager * mgr = new QNetworkAccessManager();

	QNetworkRequest request(url);

	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QJsonObject obj;
	obj["content"] = QString::fromStdString("<@" + discordID + ">");
	// 1st embed is meta info
	QJsonObject embed1;

	embed1["description"] = message;

	QJsonArray embeds;
	embeds.append(embed1);
	// 2nd embed is error message
	QJsonObject embed2;
	embed2["description"] = QString::fromStdString(error);
	embeds.append(embed2);
	obj["embeds"] = embeds;
	QNetworkReply* reply = mgr->post(request, QJsonDocument(obj).toJson(QJsonDocument::Compact));

	QEventLoop loop;
	reply->connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();

	QVariant status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	if(status_code.isValid())
	{
		if(status_code.toInt() == 200 || status_code.toInt() == 204)
		{
			// success
			QMessageBox msg;
			msg.setIcon(QMessageBox::Information);
			msg.setText(QObject::tr("Bug report sent successfully"));
			msg.setWindowTitle(QObject::tr("Success"));
			msg.exec();
			return;
		}
	}

	// if we reach this point, we failed
	QMessageBox msg;
	msg.setIcon(QMessageBox::Critical);
	msg.setText(QObject::tr("Bug report failed to send"));
	msg.setWindowTitle(QObject::tr("Error"));
	msg.exec();
}
