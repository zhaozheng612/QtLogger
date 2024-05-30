#ifndef QLOGGER_H
#define QLOGGER_H

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <QObject>
#include <QWidget>
#include <QPlainTextEdit>
#include "spdlog/spdlog.h"

class QLogger : public QWidget
{
    Q_OBJECT
public:
    explicit QLogger(QWidget *parent = nullptr);
    ~QLogger();

signals:

public slots:

private:
    //std::shared_ptr<spdlog::logger> logger;
};

#endif // QLOGGER_H
