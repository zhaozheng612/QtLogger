#include "widget.h"
#include "ui_widget.h"
#include "qlogger.h"
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <functional>
#include <string>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->resize(800, 600);   // 调整窗体大小
    this->setWindowTitle(QString::fromLocal8Bit("日志记录器"));

    // 窗体剧中
    // 获取屏幕坐标系
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    // 获取窗口坐标系
    QRect windowGeometry = frameGeometry();
    // 计算窗口的左上角坐标，以使窗口居中
    int x = (screenGeometry.width() - windowGeometry.width()) / 2;
    int y = (screenGeometry.height() - windowGeometry.height()) / 2;
    // 移动窗口到计算出的坐标
    this->move(x, y);

    // 创建日志记录器
    QLogger* logger = new QLogger(this);
    logger->setMinimumHeight(200);

    // 创建按钮面板
    auto controlPanel = new QWidget(this);
    controlPanel->setMinimumHeight(80);
    controlPanel->setMaximumHeight(200);

    QMap<QString, std::function<void(const std::string&)>> logMap = {
        {"trace", [](const std::string msg){spdlog::trace(msg);}},
        {"debug", [](const std::string msg){spdlog::debug(msg);}},
        {"info", [](const std::string msg){spdlog::info(msg);}},
        {"warn", [](const std::string msg){spdlog::warn(msg);}},
        {"error", [](const std::string msg){spdlog::error(msg);}},
        {"critical", [](const std::string msg){spdlog::critical(msg);}}
    };

    auto panel = new QHBoxLayout;

    foreach(const QString &key, logMap.keys()) {
        auto button = new QPushButton(key, controlPanel);
        connect(button, &QPushButton::clicked, [=]{
            auto func = logMap.value(key);
            func(key.toStdString());
        });
        panel->addWidget(button);
    }
    controlPanel->setLayout(panel);

    auto layout = new QVBoxLayout;
    layout->addWidget(controlPanel);
    layout->addWidget(logger);
    this->setLayout(layout);
    this->show();
}

Widget::~Widget()
{
    delete ui;
}
