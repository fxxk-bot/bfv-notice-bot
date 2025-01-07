#pragma once

#include "bfvnoticebot.h"

#include <QString>
#include <QJsonObject>


class http {

public:
	static QString get(bfvnoticebot* mainClass, const QString& url);

	static QString post(bfvnoticebot* mainClass, const QString& url, const QJsonObject& jsonData);
};

