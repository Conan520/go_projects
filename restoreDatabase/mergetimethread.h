#ifndef MERGETIMETHREAD_H
#define MERGETIMETHREAD_H
#include <QThread>
#include <iostream>
#include <windows.h>
#include <QProcess>
#include <QFile>
#include <string>
#include <set>
#include "mythread.h"
#include "logger.h"

class MergeTimeThread:public QThread
{
    Q_OBJECT
public:
    MergeTimeThread();
    ~MergeTimeThread();

    myThread* getThread();
    std::set<std::string> read_file(std::string m_file);
    void mergeDBbyTime();
    std::string get_time();
    void setDirectory(QString dir);

    void setDays(unsigned long days);
signals:
    void updateText(QString);
private:
    void run() override;
    myThread *mthread;
    QString directory;
    Logger *log;
    unsigned long days = 1;
};

#endif // MERGETIMETHREAD_H
