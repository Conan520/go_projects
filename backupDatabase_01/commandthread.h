#ifndef COMMANDTHREAD_H
#define COMMANDTHREAD_H
#include <QThread>
#include <iostream>
#include <string>
#include <QProcess>
#include "logger.h"
class commandThread:public  QThread
{
    Q_OBJECT
public:
    commandThread();
    ~commandThread();
    void setCommand(std::string command);
    void exeuteCommand(const char* command);
    void run();

signals:
    void UpdateText(QString str);
private:
    std::string command;
    Logger *log;
};

#endif // COMMANDTHREAD_H
