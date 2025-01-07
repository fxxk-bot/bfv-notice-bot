#include "http.h"
#include "bfvnoticebot.h"

#include <QString>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>



QString http::get(bfvnoticebot* mainClass, const QString& url) {
    try {

        QNetworkRequest request(url);
        QNetworkReply* reply = mainClass->manager->get(request);

        QEventLoop loop;
        QTimer timeoutTimer;
        QString response;

        timeoutTimer.setSingleShot(true);
        QObject::connect(&timeoutTimer, &QTimer::timeout, [&]() {
            reply->abort();
            });
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

        timeoutTimer.start(3000);
        loop.exec();

        if (timeoutTimer.isActive()) {
            timeoutTimer.stop();
            response = reply->readAll();
        }
        else {
            response = "Error: Timeout";
        }

        reply->deleteLater();
        return response;
    }
    catch (...) {
        return "";
    }
}


QString http::post(bfvnoticebot* mainClass, const QString& url, const QJsonObject& jsonData) {
    try {
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonDocument doc(jsonData);
        QByteArray data = doc.toJson();

        QNetworkReply* reply = mainClass->manager->post(request, data);

        QEventLoop loop;
        QTimer timeoutTimer;
        QString response;

        timeoutTimer.setSingleShot(true);
        QObject::connect(&timeoutTimer, &QTimer::timeout, [&]() {
            reply->abort();
            });
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

        timeoutTimer.start(20000);
        loop.exec();

        if (timeoutTimer.isActive()) {
            timeoutTimer.stop();
            response = reply->readAll();
        }
        else {
            response = "Error: Timeout";
        }

        reply->deleteLater();
        return response;
    }
    catch (...) {
        return "";
    }
}
