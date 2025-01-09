#include "file.h"

#include <QJsonDocument>
#include <QFile>
#include <QTextStream>


QJsonObject file::read() {

	QFile file(DEFAULT_FILE_NAME);
	QString content;

	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&file);
		in.setCodec("UTF-8");
		content = in.readAll();
		file.close();
	}

	QJsonDocument jsonDoc = QJsonDocument::fromJson(content.toUtf8());
	if (jsonDoc.isNull() || !jsonDoc.isObject()) {
		return QJsonObject();
	}
	QJsonObject jsonObj = jsonDoc.object();

	return jsonObj;
}

void file::write(QJsonObject& json) {

	QJsonDocument jsonDoc(json);
	QString jsonString = jsonDoc.toJson(QJsonDocument::Indented);

	QFile file("bfv-notice-bot.json");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		out.setCodec("UTF-8");
		out << jsonString;
		file.close();
	}
}