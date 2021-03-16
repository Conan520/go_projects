#ifndef MYTHREAD_H
#define MYTHREAD_H
#include <QThread>
#include <iostream>
#include <windows.h>
#include <QProcess>
#include "logger.h"

class myThread:public QThread
{
    Q_OBJECT
public:
    myThread();
    ~myThread();
    void setCommand(std::string command);
    void exeuteCommand(const char* command);

signals:
    void SignalUpdateText(QString str);
private:
    void run();
    Logger* log;
    std::string command;
};

#endif // MYTHREAD_H
