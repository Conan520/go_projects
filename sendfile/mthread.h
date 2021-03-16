#ifndef MTHREAD_H
#define MTHREAD_H

#include <QAbstractSocket>
#include <QThread>
#include <iostream>
#include <fstream>
#include <ostream>
#include <QString>
#include <QTcpSocket>
#include <QDate>
#include <QApplication>
#include <QFile>
#include <QDataStream>
#include <QException>
#include <exception>
#include <string>
#include "logger.h"

class mThread:public QThread
{
    Q_OBJECT
public:
    mThread();
    ~mThread();
    void setFolder(QString folder);
    QStringList read_file(std::string file);
    void setTcpSocket(QTcpSocket *send);
    void setState(int state);

signals:
    void send_file();
    void setfile(QString);
    void updateText(QString);
private:
    QString fileName;
    QString folder;
    QTcpSocket *send;
    Logger* log;
    int tcp_state;
    void run() override;
};

#endif // MTHREAD_H
