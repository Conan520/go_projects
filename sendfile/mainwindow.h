#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "mthread.h"
#include <QMainWindow>
#include <QAbstractSocket>
#include <string>
#include <vector>
#include "logger.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QByteArray;
class QFile;
class QString;
class QTcpSocket;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QStringList read_file(std::string);

signals:
    void send_socket(QTcpSocket &tcpsk);

private slots:
    void start_transfer();
    void continue_transfer(qint64);
    void show_error(QAbstractSocket::SocketError);
    QString get_time();

    void slotUpdatetext(QString);

    void on_selectBtn_clicked();

    void on_sendBtn_clicked();

    void setFileName(QString fileName);

    void on_selectFolderBtn_clicked();

    void on_autoBtn_clicked();

private:
    Ui::MainWindow *ui;
    mThread* mthread;
    QTcpSocket *send;
    QFile *file;
    QString fileName;
    QString folder;
    /* 总数据大小，已发送数据大小，剩余数据大小，每次发送数据块大小 */
    qint64 fileBytes, sentBytes, restBytes, loadBytes;
    Logger* log;
};
#endif // MAINWINDOW_H
