#include "bfvnoticebot.h"

#include "http.h"

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

bfvnoticebot::bfvnoticebot(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

	timer = new QTimer(this);
	this->manager = new QNetworkAccessManager(this);
	this->napcatUrl = "http://localhost:3000";


    connect(ui.actionNapcatConfig, &QAction::triggered, this, &bfvnoticebot::config_napcat);
    connect(ui.actionApiConfig, &QAction::triggered, this, &bfvnoticebot::config_api);

	connect(ui.actionAbout, &QAction::triggered, this, &bfvnoticebot::about);

	connect(timer, &QTimer::timeout, this, &bfvnoticebot::doTimer);

	connect(ui.pushButton, &QPushButton::clicked, this, &bfvnoticebot::clickBtn);

	connect(ui.listWidget, &QListWidget::customContextMenuRequested, this, &bfvnoticebot::showContextMenu);

	ui.listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

	ui.listWidget->addItem("服务器qq群: xxxxx");
	ui.listWidget->addItem("限杀100, 加群不限");
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

	formLayout.addRow("<p>地址: </p>", urlEdit);

	QPushButton* testButton = new QPushButton("测试", &dialog);
	QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		Qt::Horizontal, &dialog);

	buttonBox.addButton(testButton, QDialogButtonBox::ActionRole);

	formLayout.addRow(&buttonBox);

	connect(testButton, &QPushButton::clicked, [&]() {
		QString url = urlEdit->text();

		if (url.isEmpty()) {
			QMessageBox::warning(&dialog, "提示", "地址不能为空！");
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
		jsonData["user_id"] = 3889013937;
		jsonData["message"] = "/chat 公告机测试";

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

		if (url.isEmpty()) {
			QMessageBox::warning(&dialog, "提示", "地址不能为空！");
			continue;
		}
		this->napcatUrl = url;
		break;
	}
}


void bfvnoticebot::clickBtn() {

	auto text = ui.pushButton->text();

	if (text == "开始") {
		ui.pushButton->setText("停止");

		if (ui.apiRadioButton->isChecked()) {
			if (this->apiRemid.isEmpty() || this->apiSid.isEmpty()) {
				QMessageBox::warning(this, "提示", "remid或sid不能为空！请配置相关参数");
				return;
			}
		}
		
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
		else if (ui.napcatRadioButton->isChecked()) {
			QJsonObject jsonData;
			jsonData["user_id"] = 3889013937;
			jsonData["message"] = "/chat " + itemText;

			auto resp = http::post(this, this->napcatUrl + "/send_private_msg", jsonData);

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

		QThread::msleep(round * 1000);
	}
}



void bfvnoticebot::about() {
	QMessageBox msgBox(this);
	msgBox.setWindowTitle("关于");
	msgBox.setText("<p>项目已开源</p><p> <a href='https://github.com/fxxk-bot/bfv-notice-bot'>https://github.com/fxxk-bot/bfv-notice-bot</a></p>");
	msgBox.exec();
}

