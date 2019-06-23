#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDebug>
#include <QJsonArray>
#include <QJsonValue>
#include <QIntValidator>
#include <QFileInfo>
#include <QProcess>
#include <unistd.h>
#include <ui_PrefrencesWindow.h>

#include "utils.h"
#include "PrefrencesWindow.h"

PrefrencesWindow::PrefrencesWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrefrencesWindow)
{
    ui->setupUi(this);
    rootObj = loadRootObjFromConf();
    QJsonObject http = findValueFromJsonArray(rootObj.value("inbounds").toArray(), "tag", "http-in");
    QJsonObject socks = findValueFromJsonArray(rootObj.value("inbounds").toArray(), "tag", "socks-in");
    if(rootObj.value("v2suidEnabled").toBool()) {
        ui->runAsRootCheckBox->setCheckState(Qt::Checked);
    }
    if(!http.isEmpty()) {
        ui->httpPortLE->setText(http.value("port").toString());
        ui->httpCB->setCheckState(Qt::Checked);
    } else {
        ui->httpPortLE->setDisabled(true);
    }
    if(!socks.isEmpty()) {
        ui->socksPortLE->setText(socks.value("port").toString());
        ui->socksCB->setCheckState(Qt::Checked);
    } else {
        ui->socksPortLE->setDisabled(true);
    }
    ui->httpPortLE->setValidator(new QIntValidator());
    ui->socksPortLE->setValidator(new QIntValidator());
    parentMW = parent;
}

PrefrencesWindow::~PrefrencesWindow()
{
    delete ui;
}

void PrefrencesWindow::on_buttonBox_accepted()
{
    if(checkVCoreExes()) {
        if(ui->httpPortLE->text().toInt() != ui->socksPortLE->text().toInt()) {
            QJsonArray inbounds;
            QJsonDocument modifiedDoc;
            inbounds = rootObj.value("inbounds").toArray();
            int socksId = getIndexByValue(inbounds, "tag", "socks-in");
            if(socksId != -1) {
                inbounds.removeAt(socksId);
            }
            int httpId = getIndexByValue(inbounds, "tag", "http-in");
            if(httpId != -1) {
                inbounds.removeAt(httpId);
            }
            rootObj.remove("inbounds");
            rootObj.remove("v2suidEnabled");
            if(ui->socksCB->isChecked()) {
                QJsonObject socks;
                QJsonObject settings;
                socks.insert("tag", "socks-in");
                socks.insert("port", ui->socksPortLE->text().toInt());
                socks.insert("listen", "127.0.0.1");
                socks.insert("protocol", "socks");
                settings.insert("auth", "noauth");
                settings.insert("udp", true);
                settings.insert("ip", "127.0.0.1");
                socks.insert("settings", QJsonValue(settings));
                inbounds.append(socks);
            }
            if(ui->httpCB->isChecked()) {
                QJsonObject http;
                QJsonObject settings;
                http.insert("tag", "http-in");
                http.insert("port", ui->httpPortLE->text().toInt());
                http.insert("listen", "127.0.0.1");
                http.insert("protocol", "http");
                settings.insert("auth", "noauth");
                settings.insert("udp", true);
                settings.insert("ip", "127.0.0.1");
                http.insert("settings", QJsonValue(settings));
                inbounds.append(http);
            }
            rootObj.insert("inbounds", QJsonValue(inbounds));
#ifndef _WIN32
            // Set UID and GID in *nix
            QFileInfo v2rayCoreExeFile("v2ray");
            if(ui->checkBox->isChecked() && v2rayCoreExeFile.ownerId() != 0) {
                QProcess::execute("pkexec", QStringList() << "bash" << "-c" << "chown root:root " + QCoreApplication::applicationDirPath() + "/v2ray" + ";chmod +s " + QCoreApplication::applicationDirPath() + "/v2ray");
            } else if (!ui->checkBox->isChecked() && v2rayCoreExeFile.ownerId() == 0) {
                uid_t uid = getuid();
                gid_t gid = getgid();
                QProcess::execute("pkexec", QStringList() << "chown" << QString::number(uid) + ":" + QString::number(gid) << QCoreApplication::applicationDirPath() + "/v2ray");
            }
            v2rayCoreExeFile.refresh();
            rootObj.insert("v2suidEnabled", v2rayCoreExeFile.ownerId() == 0);
#else
            // No such uid gid thing on windows....
#endif
            modifiedDoc.setObject(rootObj);
            QByteArray byteArray = modifiedDoc.toJson(QJsonDocument::Indented);
            QFile confFile("conf/Hv2ray.config.json");
            if(!confFile.open(QIODevice::WriteOnly)) {
                showWarnMessageBox(this, tr("#Prefrences"), tr("#CannotOpenConfigFile"));
                qDebug() << "Cannot open Hv2ray.config.json for modifying";
            }
            confFile.write(byteArray);
            confFile.close();
        } else {
            showWarnMessageBox(this, tr("Prefrences"), tr("PortNumbersCannotBeSame"));
        }
    }
}


void PrefrencesWindow::on_httpCB_stateChanged(int checked)
{
    if(checked != Qt::Checked) {
        ui->httpPortLE->setDisabled(true);
    } else {
        ui->httpPortLE->setEnabled(true);
        ui->httpPortLE->setText("6666");
    }
}

void PrefrencesWindow::on_socksCB_stateChanged(int checked)
{
    if(checked != Qt::Checked) {
        ui->socksPortLE->setDisabled(true);
    } else {
        ui->socksPortLE->setEnabled(true);
        ui->socksPortLE->setText("1080");
    }
}

void PrefrencesWindow::on_httpAuthCB_stateChanged(int checked)
{
    if(checked)
    {

    }
}

void PrefrencesWindow::on_runAsRootCheckBox_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
#ifdef _WIN32
    showWarnMessageBox(this, tr("Prefrences"), tr("RunAsRootNotOnWindows"));
#endif
}