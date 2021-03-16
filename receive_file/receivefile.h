#ifndef RECEIVEFILE_H
#define RECEIVEFILE_H

#include "logger.h"
#include <QMainWindow>
#include <QAbstractSocket>
class QFile;
class QString;
class QTcpServer;
class QTcpSocket;

QT_BEGIN_NAMESPACE
namespace Ui { class ReceiveFile; }
QT_END_NAMESPACE

class ReceiveFile : public QMainWindow
{
    Q_OBJECT

public:
    ReceiveFile(QWidget *parent = nullptr);
    ~ReceiveFile();

private slots:
    void accept_connect();
    void recevie_file();
    void show_error(QAbstractSocket::SocketError);
    void on_listenBtn_clicked();
    QString get_time();

    void on_selectDirBtn_clicked();

private:
    Ui::ReceiveFile *ui;
    QTcpServer *server;
    QTcpSocket *receive;
    QString fileName;
    QString directory;
    QFile *file;
    QFile *destFile;
    /* 已接受数据，总数据，文件名长度 */
    qint64 gotBytes, fileBytes, nameSize;
    Logger *log;
};
#endif // RECEIVEFILE_H
