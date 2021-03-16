#include "logger.h"

Logger::Logger()
{
    logFile = new QFile("log.txt");
}

Logger::~Logger()
{
    if (logFile)
        delete logFile;
}

void Logger::appendLog(QString text)
{
    QString now = getTime();
    if (logFile->open(QIODevice::WriteOnly|QIODevice::Append)) {
        logFile->write(QString("%1 %2").arg(now).arg(text).toStdString().c_str());
        logFile->write("\n");
        logFile->close();
    }
    size_t maxSize = 2 * 1024 * 1024 * 1024;
    if (size_t(logFile->size()) > maxSize) {
        if (logFile->open(QIODevice::WriteOnly)) {

        }
    }
}

QString Logger::getTime()
{
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss :");
    return current_date_time;
}
