#include "qlogger.h"
#include <spdlog/async.h>
#include <spdlog/sinks/qt_sinks.h>
#include "tcp_server_sink.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <QHBoxLayout>
#include <QTextEdit>

QLogger::QLogger(QWidget *parent)
    : QWidget(parent)
{
    auto textEdit = new QTextEdit(this);

    auto layout = new QHBoxLayout;
    layout->addWidget(textEdit);
    this->setLayout(layout);
    this->show();

    spdlog::init_thread_pool(8192, 1);
    spdlog::flush_every(std::chrono::seconds(3));

    // QTextEdit
    auto qt_sink = std::make_shared<spdlog::sinks::qt_color_sink_mt>(textEdit, 500, false, true);
    qt_sink->set_level(spdlog::level::trace);

    // tcp
    const std::string host = "127.0.0.1";
    const int port = 9000;
    auto tcp_server_sink = std::make_shared<spdlog::sinks::tcp_server_sink_mt>(host, port);
    tcp_server_sink->set_level(spdlog::level::trace);

    // text file
    auto daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/log.txt", 0, 0);
    daily_sink->set_level(spdlog::level::trace);

    std::vector<spdlog::sink_ptr> sinks{qt_sink, daily_sink, tcp_server_sink};
    auto logger = std::make_shared<spdlog::async_logger>("log", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    logger->set_level(spdlog::level::trace);
    //logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    //logger->set_pattern("[%H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    spdlog::set_default_logger(logger);
}

QLogger::~QLogger()
{
    spdlog::shutdown();
}
