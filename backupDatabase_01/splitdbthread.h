#ifndef SPLITDBTHREAD_H
#define SPLITDBTHREAD_H
#include <QThread>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <time.h>
#include <QString>
#include <math.h>
#include <commandthread.h>
#include <QDate>
#include <QApplication>
#include <QFile>
#include <QDataStream>
#include <QException>
#include <exception>
#include <string>
#include "logger.h"

class splitDbThread:public QThread
{
    Q_OBJECT
public:
    splitDbThread();
    ~splitDbThread();
    void setdbPath(std::string);
    void setfolder(QString folder);
    void set_baksize(size_t size);
    void splitDataBase(const char* dbPath);
    commandThread* getThread();
signals:
    void updateMessage(QString);
private:
    std::string dbPath;
    QString folder;
    size_t bak_size = 1024;
    commandThread* comThread;
    Logger* log;
    void run() override;
};

#endif // SPLITDBTHREAD_H
