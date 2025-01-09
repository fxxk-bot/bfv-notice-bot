#pragma once
#include "ui_bfvnoticebot.h"

#include <QtWidgets/QMainWindow>
#include <QPoint>
#include <QNetworkAccessManager>
#include <QTimer>

class bfvnoticebot : public QMainWindow
{
	Q_OBJECT

public:
	bfvnoticebot(QWidget* parent = nullptr);
	~bfvnoticebot();

private slots:
	void config_napcat();

	void config_api();

	void about();

	void showContextMenu(const QPoint& pos);

	void addItemToList();

	void clickBtn();

	void doTimer();

	void ignoreApiError();

	void spinBoxValueChange(int);

	void radioChecked(bool);

public:
	QNetworkAccessManager* manager;

private:
	Ui::bfvnoticebotClass ui;
	QString napcatUrl;
	QString napcatQq;
	QString apiRemid;
	QString apiSid;
	QTimer* timer = nullptr;

private:
	void saveConfig();

	void readConfig();
};
