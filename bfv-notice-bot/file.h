
#include <QString>
#include <QJsonObject>

#define DEFAULT_FILE_NAME "bfv-notice-bot.json"

#pragma once
class file
{

public:

	static QJsonObject read();

	static void write(QJsonObject& json);
};

