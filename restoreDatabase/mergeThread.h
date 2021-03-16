#ifndef MERGETHREAD_H
#define MERGETHREAD_H
#include <QThread>
#include <iostream>
#include <windows.h>
#include <QProcess>
#include <string>
#include <set>
#include <QFile>
#include "logger.h"

class mergeThread: public QThread
{
    Q_OBJECT
public:
    mergeThread();
    ~mergeThread();
    void setDir(QString dir);
    QStringList read_file(std::string);
    void mergeFile();
signals:
    void updateText(QString);
private:
    void run() override;
    QString directory;
    Logger *log;
};
#endif // MERGETHREAD_H
