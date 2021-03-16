#ifndef MYTHREAD_H
#define MYTHREAD_H
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

class MyThread :public QThread
{
    Q_OBJECT
public:
    MyThread();
    ~MyThread();
    size_t GetFileSize(const std::string &file_name);
    void splitDatabaseByTime(std::string dbPath);
    void splitDatabaseBySize(std::string dbPath);
    void nbkFileSplit(const char *fileName);
    void setSign(bool sign);
    void setDays(int days);
    void setPath(std::string dbPath);
    void setSplitSize();
    void setSize(size_t size);
    void setFolder(QString folder);
    void setKind(int kind);     //设置运行哪个函数
    commandThread* getComthread();

signals:
    void SignalUpdateText(QString str);

private:
    void run();
    bool sign = true;
    int kind = 0;
    int days = 0; 
    QString folder;
    size_t bak_size = 1024;
    std::string dbPath;
    commandThread *comThread;
    Logger *log;

};

#endif // MYTHREAD_H
