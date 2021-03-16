#ifndef LOGGER_H
#define LOGGER_H

#include <QDate>
#include <QApplication>
#include <QFile>
#include <QException>
#include <math.h>
class Logger
{
public:
    Logger();
    ~Logger();
    void appendLog(QString text);
    QString getTime();
private:
    QFile *logFile;
};
#endif // LOGGER_H
