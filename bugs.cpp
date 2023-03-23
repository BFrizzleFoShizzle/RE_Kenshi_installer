#include "bugs.h"

#include "discord.h"

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

std::string installerVersion = "0.1";

void Bugs::ReportBug(std::string window, int step, std::string error)
{
    // get user consent
    QMessageBox consentBox;
    consentBox.setIcon(QMessageBox::Critical);
    consentBox.setWindowTitle(QObject::tr("Error"));
    consentBox.setText(QString(QObject::tr("RE_Kenshi's installer has encountered an error"))
                       + QObject::tr("\nWould you like to send an error report to the RE_Kenshi team?")
                       + QObject::tr("\nYour report will be sent to RE_Kenshi's developer (BFrizzleFoShizzle) with the following information:")
                       + QObject::tr("\nInstaller version: ") + QString::fromStdString(installerVersion)
                       + QObject::tr("\nWindow: ") + QString::fromStdString(window) // the name of the window - e.g. InstallerWindow, UninstallerWindow
                       + QObject::tr("\nInstallation step: ") + QString::number(step)
                       + QObject::tr("\nError message: ")
                       + QObject::tr("\n") + QString::fromStdString(error));
    consentBox.setStandardButtons(QMessageBox::Yes);
    consentBox.addButton(QMessageBox::No);
    consentBox.setDefaultButton(QMessageBox::Yes);

    if(consentBox.exec() != QMessageBox::Yes)
        return;

    // We have consent to upload error report

    QUrl url = QUrl(QString::fromStdString(installerBugDiscordWebHookURL));
    QNetworkAccessManager * mgr = new QNetworkAccessManager();

    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["content"] = QString::fromStdString("<@" + discordID + ">");
    QJsonArray embeds;
    // 1st embed is meta info
    QJsonObject embed1;
    embed1["description"] = QString::fromStdString("Installer " + installerVersion + " error:\nWindow: " + window
            + "\nStep: " + std::to_string(step) + "\nError message:");
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