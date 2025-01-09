#include "bfvnoticebot.h"

#include "http.h"
#include "file.h"

#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QThread>
#include <QJsonArray>




bfvnoticebot::bfvnoticebot(QWidget* parent)
	: QMainWindow(parent) {
	ui.setupUi(this);

	timer = new QTimer(this);

	this->manager = new QNetworkAccessManager(this);

	this->readConfig();

	connect(ui.actionNapcatConfig, &QAction::triggered, this, &bfvnoticebot::config_napcat);
	connect(ui.actionApiConfig, &QAction::triggered, this, &bfvnoticebot::config_api);

	connect(ui.actionAbout, &QAction::triggered, this, &bfvnoticebot::about);
	connect(ui.actionIgnoreError, &QAction::triggered, this, &bfvnoticebot::ignoreApiError);

	connect(timer, &QTimer::timeout, this, &bfvnoticebot::doTimer);

	connect(ui.pushButton, &QPushButton::clicked, this, &bfvnoticebot::clickBtn);

	connect(ui.listWidget, &QListWidget::customContextMenuRequested, this, &bfvnoticebot::showContextMenu);


	ui.listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
}

bfvnoticebot::~bfvnoticebot()
{}


void bfvnoticebot::showContextMenu(const QPoint& pos) {

	QListWidgetItem* item = ui.listWidget->itemAt(pos);
	if (item) {
		QMenu contextMenu(this);
		QAction* deleteAction = contextMenu.addAction("删除");

		QAction* selectedAction = contextMenu.exec(ui.listWidget->viewport()->mapToGlobal(pos));
		if (selectedAction == deleteAction) {
			delete item;
			this->saveConfig();
		}
		return;
	}

	QMenu contextMenu(this);
	QAction* addAction = contextMenu.addAction("新增");

	QAction* selectedAction = contextMenu.exec(ui.listWidget->viewport()->mapToGlobal(pos));
	if (selectedAction == addAction) {
		addItemToList();
	}
}


void bfvnoticebot::addItemToList() {
	bool ok;
	QString text = QInputDialog::getText(this, "新增", "请输入", QLineEdit::Normal, "", &ok);

	if (ok && !text.isEmpty()) {
		ui.listWidget->addItem(text);
		this->saveConfig();
	}
}

void bfvnoticebot::config_api() {

	QDialog dialog(this);
	dialog.setWindowTitle("API配置");
	dialog.setMinimumWidth(400);

	QFormLayout formLayout(&dialog);

	QLineEdit* remidEdit = new QLineEdit(&dialog);
	remidEdit->setPlaceholderText("请填写remid");
	remidEdit->setText(this->apiRemid);

	QLineEdit* sidEdit = new QLineEdit(&dialog);
	sidEdit->setPlaceholderText("请填写sid");
	sidEdit->setText(this->apiSid);


	formLayout.addRow("<p>remid: </p>", remidEdit);
	formLayout.addRow("<p>sid: </p>", sidEdit);

	QPushButton* testButton = new QPushButton("测试", &dialog);
	QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		Qt::Horizontal, &dialog);

	buttonBox.addButton(testButton, QDialogButtonBox::ActionRole);

	formLayout.addRow(&buttonBox);

	connect(testButton, &QPushButton::clicked, [&]() {
		QString remid = remidEdit->text();
		QString sid = sidEdit->text();

		if (remid.isEmpty()) {
			QMessageBox::warning(&dialog, "提示", "remid不能为空！");
			return;
		}
		if (sid.isEmpty()) {
			QMessageBox::warning(&dialog, "提示", "sid不能为空！");
			return;
		}


		QJsonObject jsonData;
		jsonData["remid"] = remid;
		jsonData["sid"] = sid;
		jsonData["content"] = "公告机测试";
		jsonData["gameId"] = 0;

		QString url = "https://api.bfvrobot.net/api/player/sendMessage";

		QMessageBox msgBox(&dialog);
		msgBox.setWindowTitle("提示");
		msgBox.setText("正在请求");
		msgBox.setIcon(QMessageBox::Information);
		msgBox.setStandardButtons(QMessageBox::NoButton);
		msgBox.show();

		QApplication::processEvents();

		QString resp = http::post(this, url, jsonData);

		msgBox.close();


		QJsonDocument jsonDoc = QJsonDocument::fromJson(resp.toUtf8());
		if (jsonDoc.isNull() || !jsonDoc.isObject()) {
			QMessageBox::warning(&dialog, "测试", QString("失败: 无效的响应"));
			return;
		}
		QJsonObject jsonObj = jsonDoc.object();
		QString code = jsonObj.value("code").toString();

		if (code != "sendMessage.success") {
			if (code == "sendMessage.invalid_cookies") {
				QMessageBox::warning(&dialog, "测试", QString("cookies失效, 响应:\n%1").arg(resp));
			}
			else {
				QMessageBox::warning(&dialog, "测试", QString("失败, 响应:\n%1").arg(resp));
			}
			return;
		}

		QMessageBox::information(&dialog, "测试", QString("成功, 响应:\n%1").arg(resp));
		});

	connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	while (1 && dialog.exec() == QDialog::Accepted) {

		QString remid = remidEdit->text();
		QString sid = sidEdit->text();

		if (remid.isEmpty()) {
			QMessageBox::warning(&dialog, "提示", "remid不能为空！");
			continue;
		}

		if (sid.isEmpty()) {
			QMessageBox::warning(&dialog, "提示", "sid不能为空！");
			continue;
		}

		this->apiRemid = remid;
		this->apiSid = sid;
		break;
	}
}



void bfvnoticebot::config_napcat() {

	QDialog dialog(this);
	dialog.setWindowTitle("NapCat配置");
	dialog.setMinimumWidth(400);

	QFormLayout formLayout(&dialog);

	QLineEdit* urlEdit = new QLineEdit(&dialog);
	urlEdit->setText(this->napcatUrl);
	urlEdit->setPlaceholderText("请填写NapCat地址");

	QLineEdit* qqEdit = new QLineEdit(&dialog);
	qqEdit->setText(this->napcatQq);
	qqEdit->setPlaceholderText("请填写哔哔机QQ号");

	formLayout.addRow("<p>地址: </p>", urlEdit);
	formLayout.addRow("<p>哔哔机QQ号: </p>", qqEdit);

	QPushButton* testButton = new QPushButton("测试", &dialog);
	QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		Qt::Horizontal, &dialog);

	buttonBox.addButton(testButton, QDialogButtonBox::ActionRole);

	formLayout.addRow(&buttonBox);

	connect(testButton, &QPushButton::clicked, [&]() {
		QString url = urlEdit->text();
		QString qq = qqEdit->text();

		if (url.isEmpty()) {
			QMessageBox::warning(&dialog, "提示", "地址不能为空！");
			return;
		}

		if (qq.isEmpty()) {
			QMessageBox::warning(&dialog, "提示", "哔哔机QQ号不能为空！");
			return;
		}

		bool ok;

		unsigned long qqNumber = qq.toULong(&ok);

		if (!ok) {
			QMessageBox::warning(&dialog, "提示", "哔哔机QQ号格式不正确！");
			return;
		}

		QMessageBox msgBox(&dialog);
		msgBox.setWindowTitle("提示");
		msgBox.setText("正在请求");
		msgBox.setIcon(QMessageBox::Information);
		msgBox.setStandardButtons(QMessageBox::NoButton);
		msgBox.show();

		QApplication::processEvents();

		QJsonObject jsonData;
		jsonData["user_id"] = qq;
		jsonData["message"] = "公告机测试";

		auto resp = http::post(this, url + "/send_private_msg", jsonData);

		msgBox.close();

		QJsonDocument jsonDoc = QJsonDocument::fromJson(resp.toUtf8());
		if (jsonDoc.isNull() || !jsonDoc.isObject()) {
			QMessageBox::warning(&dialog, "测试", QString("失败: 无效的响应"));
			return;
		}
		QJsonObject jsonObj = jsonDoc.object();
		int retcode = jsonObj.value("retcode").toInt();
		QString status = jsonObj.value("status").toString();

		if (retcode != 0 || status != "ok") {
			QMessageBox::warning(&dialog, "测试", QString("失败, 响应:\n%1").arg(resp));
			return;
		}

		QMessageBox::information(&dialog, "测试", QString("成功:\n%1").arg(resp));
		});

	connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	while (1 && dialog.exec() == QDialog::Accepted) {

		QString url = urlEdit->text();
		QString qq = qqEdit->text();

		if (url.isEmpty() || qq.isEmpty()) {
			QMessageBox::warning(&dialog, "提示", "地址或哔哔机QQ号不能为空！");
			continue;
		}

		bool ok;

		unsigned long qqNumber = qq.toULong(&ok);

		if (!ok) {
			QMessageBox::warning(&dialog, "提示", "哔哔机QQ号格式不正确！");
			continue;
		}

		this->napcatUrl = url;
		this->napcatQq = qq;
		break;
	}
	this->saveConfig();
}


void bfvnoticebot::clickBtn() {

	auto text = ui.pushButton->text();

	if (text == "开始") {

		if (ui.apiRadioButton->isChecked()) {
			if (this->apiRemid.isEmpty() || this->apiSid.isEmpty()) {
				QMessageBox::warning(this, "提示", "remid或sid不能为空！请配置相关参数");
				return;
			}
		}

		ui.pushButton->setText("停止");


		int round = ui.roundSpinBox->value();

		this->timer->start(round * 1000);
	}
	else if (text == "停止") {
		ui.pushButton->setText("开始");

		this->timer->stop();
	}
}


void bfvnoticebot::doTimer() {

	int round = ui.multiSpinBox->value();

	int itemCount = ui.listWidget->count();

	auto text = ui.actionIgnoreError->text();

	bool showError = text == "忽略接口错误";

	for (int i = 0; i < itemCount; ++i) {
		QListWidgetItem* item = ui.listWidget->item(i);

		QString itemText = item->text();

		if (ui.apiRadioButton->isChecked()) {
			QJsonObject jsonData;
			jsonData["remid"] = this->apiRemid;
			jsonData["sid"] = this->apiSid;
			jsonData["content"] = itemText;
			jsonData["gameId"] = 0;
			QString url = "https://api.bfvrobot.net/api/player/sendMessage";
			QString resp = http::post(this, url, jsonData);

			if (showError) {
				QJsonDocument jsonDoc = QJsonDocument::fromJson(resp.toUtf8());
				if (jsonDoc.isNull() || !jsonDoc.isObject()) {
					QMessageBox::warning(this, "测试", QString("失败: 无效的响应"));
					return;
				}
				QJsonObject jsonObj = jsonDoc.object();
				QString code = jsonObj.value("code").toString();

				if (code != "sendMessage.success") {
					if (code == "sendMessage.invalid_cookies") {
						QMessageBox::warning(this, "测试", QString("cookies失效, 响应:\n%1").arg(resp));
					}
					else {
						QMessageBox::warning(this, "测试", QString("失败, 响应:\n%1").arg(resp));
					}
					return;
				}
			}

		}
		else if (ui.napcatRadioButton->isChecked()) {
			QJsonObject jsonData;
			jsonData["user_id"] = this->napcatQq;
			jsonData["message"] = itemText;

			auto resp = http::post(this, this->napcatUrl + "/send_private_msg", jsonData);

			if (showError) {
				QJsonDocument jsonDoc = QJsonDocument::fromJson(resp.toUtf8());
				if (jsonDoc.isNull() || !jsonDoc.isObject()) {
					QMessageBox::warning(this, "测试", QString("失败: 无效的响应"));
					return;
				}
				QJsonObject jsonObj = jsonDoc.object();
				int retcode = jsonObj.value("retcode").toInt();
				QString status = jsonObj.value("status").toString();

				if (retcode != 0 || status != "ok") {
					QMessageBox::warning(this, "测试", QString("失败, 响应:\n%1").arg(resp));
					return;
				}
			}
		}

		QThread::msleep(round * 1000);
	}
}


void bfvnoticebot::spinBoxValueChange(int) {
	this->saveConfig();
}

void bfvnoticebot::radioChecked(bool) {
	this->saveConfig();
}


void bfvnoticebot::ignoreApiError() {
	auto text = ui.actionIgnoreError->text();
	if (text == "忽略接口错误") {
		ui.actionIgnoreError->setText("提示接口错误");
	}
	else if (text == "提示接口错误") {
		ui.actionIgnoreError->setText("忽略接口错误");
	}
}

void bfvnoticebot::about() {
	QMessageBox msgBox(this);
	msgBox.setWindowTitle("关于");
	msgBox.setText("<p>项目已开源</p><p> <a href='https://github.com/fxxk-bot/bfv-notice-bot'>https://github.com/fxxk-bot/bfv-notice-bot</a></p>");
	msgBox.exec();
}

void bfvnoticebot::readConfig() {

	auto json = file::read();

	auto fileNapcatUrl = json.value("napcatUrl").toString();

	if (fileNapcatUrl.isEmpty()) {
		this->napcatUrl = "http://localhost:3000";
	}
	else {
		this->napcatUrl = fileNapcatUrl;
	}

	auto fileNapcatQq = json.value("napcatQq").toString();

	if (fileNapcatQq.isEmpty()) {
		this->napcatQq = "3889363571";
	}
	else {
		this->napcatQq = fileNapcatQq;
	}

	QJsonArray jsonArray = json.value("msgList").toArray();

	if (jsonArray.size() == 0) {
		ui.listWidget->addItem("服务器qq群: xxxxx");
		ui.listWidget->addItem("限杀100, 加群不限");
	}
	else {
		for (int i = 0; i < jsonArray.size(); ++i) {
			QJsonValue value = jsonArray[i];
			ui.listWidget->addItem(value.toString());
		}
	}

	auto interval = json.value("interval").toInt();

	if (interval == 0) {
		ui.roundSpinBox->setValue(120);
	}
	else {
		ui.roundSpinBox->setValue(interval);
	}

	if (json.contains("multiMsgInterval")) {
		auto multiMsgInterval = json.value("multiMsgInterval").toInt();
		ui.multiSpinBox->setValue(multiMsgInterval);
	}
	else {
		ui.multiSpinBox->setValue(2);
	}


	if (json.contains("mode")) {
		auto mode = json.value("mode").toString();
		if (mode == "api") {
			ui.apiRadioButton->setChecked(true);
		}
		else if (mode == "napcat") {
			ui.napcatRadioButton->setChecked(true);
		}
	}
	else {
		ui.apiRadioButton->setChecked(true);
	}

	connect(ui.roundSpinBox,
		static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
		this,
		&bfvnoticebot::spinBoxValueChange);

	connect(ui.multiSpinBox,
		static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
		this,
		&bfvnoticebot::spinBoxValueChange);

	connect(ui.napcatRadioButton, &QRadioButton::toggled, this, &bfvnoticebot::radioChecked);
}

void bfvnoticebot::saveConfig() {
	QJsonObject jsonData;
	jsonData["napcatQq"] = this->napcatQq;
	jsonData["napcatUrl"] = this->napcatUrl;


	int itemCount = ui.listWidget->count();
	QJsonArray jsonArray;
	for (int i = 0; i < itemCount; ++i) {
		QListWidgetItem* item = ui.listWidget->item(i);
		jsonArray.append(item->text());
	}

	jsonData["msgList"] = jsonArray;
	jsonData["interval"] = ui.roundSpinBox->value();
	jsonData["multiMsgInterval"] = ui.multiSpinBox->value();


	if (ui.apiRadioButton->isChecked()) {
		jsonData["mode"] = "api";
	}
	else if (ui.napcatRadioButton->isChecked()) {
		jsonData["mode"] = "napcat";
	}

	file::write(jsonData);
}