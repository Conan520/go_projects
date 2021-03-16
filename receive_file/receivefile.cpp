#include "receivefile.h"
#include "ui_receivefile.h"
#include <iostream>
#include <fstream>
#include <QDataStream>
#include <QFile>
#include <QFileDialog>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QException>
#include <exception>
#include <QMessageBox>
#include <windows.h>
#include <QDateTime>

const quint16 PORT = 9030;      //默认端口号
const int DATA_STREAM_VERSION = QDataStream::Qt_5_12;


ReceiveFile::ReceiveFile(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ReceiveFile)
{
    ui->setupUi(this);
    this->setWindowTitle("接收文件");
    ui->portLineEdit->setAlignment(Qt::AlignHCenter);
    ui->portLineEdit->setText(QString::fromStdString(std::to_string(PORT)));
    ui->StatusText->setReadOnly(true);
    /* 进度条调零 */
    ui->recvProg->setValue(0);
    /* 启用监听按钮 */
    ui->listenBtn->setEnabled(true);
    fileBytes = gotBytes = nameSize = 0;
    file = Q_NULLPTR;
    receive = Q_NULLPTR;
    server = new QTcpServer(this);
    fileName = nullptr;
    destFile = Q_NULLPTR;
    log = new Logger ();
    /* 连接请求 -> 接受连接 */
    connect(server, SIGNAL(newConnection()), this, SLOT(accept_connect()));
}

ReceiveFile::~ReceiveFile()
{
    delete ui;
    delete server;
}

QString ReceiveFile::get_time()
{
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss :");
    return current_date_time;
}

void ReceiveFile::accept_connect()
{
    try {
        if (receive != nullptr) {
            delete receive;
            receive = nullptr;
        }
        receive = server->nextPendingConnection();
        /* 有数据到 -> 接受数据 */
        connect(receive, SIGNAL(readyRead()), this, SLOT(recevie_file()));
        /* socket出错 -> 出错处理 */
        connect(receive, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(show_error(QAbstractSocket::SocketError)));
        ui->StatusText->appendPlainText(QString("%1 Connection Established!").arg(get_time()));
        log->appendLog("Connection Established");
        gotBytes = 0;
        server->close();
    } catch (QException *e) {
        ui->StatusText->appendPlainText(QString("%1 %2").arg(get_time()).arg(e->what()));
        log->appendLog(e->what());
    } catch (std::exception *e) {
        ui->StatusText->appendPlainText(QString("%1 %2").arg(get_time()).arg(e->what()));
        log->appendLog(e->what());
    }
}

//接收文件
void ReceiveFile::recevie_file()
{
    try {
        QDataStream in(receive);
        in.setVersion(DATA_STREAM_VERSION);
       /* 首部未接收/未接收完 */
       if(gotBytes <= 2 * sizeof(qint64))
       {
           if(!nameSize) // 前两个长度字段未接收
           {
               if(receive->bytesAvailable() >= 2 * sizeof(qint64))
               {
                   in >> fileBytes >> nameSize;
                   gotBytes += 2 * sizeof(qint64);
                   ui->recvProg->setMaximum(fileBytes);
                   ui->recvProg->setValue(gotBytes);
               }
               else // 数据不足，等下次
                  return;
           }
           else if(receive->bytesAvailable() >= nameSize)
           {
               in >> fileName;
               fileName = directory + "/" + fileName;
               gotBytes += nameSize;
               ui->recvProg->setValue(gotBytes);
               ui->StatusText->appendPlainText(QString("%1 --- File Name: %2").arg(get_time()).arg(fileName));
           }
           else // 数据不足文件名长度，等下次
               return;
       }

       /* 已读文件名、文件未打开 -> 尝试打开文件 */
       if(!fileName.isEmpty() && file == Q_NULLPTR)
       {
           file = new QFile(fileName);
           if(!file->open(QFile::WriteOnly)) // 打开失败
           {
               //std::cerr << "*** File Open Failed ***" << std::endl;
               ui->StatusText->appendPlainText(QString("%1 *** File Open Failed ***").arg(get_time()));
               log->appendLog("*** File Open Failed ***");
               delete file;
               file = Q_NULLPTR;
               return;
           }
           ui->StatusText->appendPlainText(QString("%1 Open %2 Successfully!").arg(get_time()).arg(fileName));
           log->appendLog(QString("Open %1 Successfully !").arg(fileName));
       }
       if(file == Q_NULLPTR) // 文件未打开，不能进行后续操作
           return;

       if(gotBytes < fileBytes) // 文件未接收完
       {
           gotBytes += receive->bytesAvailable();
           ui->recvProg->setValue(gotBytes);
           file->write(receive->readAll());
       }
       if(gotBytes == fileBytes) // 文件接收完
       {
           if (fileName.contains(QRegExp("00_\\d\\d.nbk"))) {
               destFile = new QFile(directory + "/" + "nbk_00_out.txt");
               if (!destFile->open(QIODevice::WriteOnly|QIODevice::Append))
                   return;
           }
           else if (fileName.contains("nbk")) {
               destFile = new QFile(directory + "/" + "nbk_out.txt");
               if (!destFile->open(QIODevice::WriteOnly|QIODevice::Append))
                   return;
               // 表示第一个完整备份文件(多个小文件组成)的传输已经完成，可以开始合并
               QFile temp_state_file(directory + "/" + "nbk_00_state.txt");
               if (temp_state_file.open(QIODevice::WriteOnly)) {
                   temp_state_file.write("1");
                   temp_state_file.close();
               }
           }
           else if (fileName.contains("fbk")) {
               destFile = new QFile(directory + "/" + "gbak_out.txt");
               if (!destFile->open(QIODevice::WriteOnly|QIODevice::Append))
                   return;
           }
           receive->close(); // 关socket
           file->close(); // 关文件
           ui->StatusText->appendPlainText(QString("%1 Finish receiving %2").arg(get_time()).arg(fileName));
           this->repaint();
           log->appendLog(QString("Finish receiving %1").arg(fileName));
           destFile->write(fileName.toStdString().c_str());
           destFile->write("\n");

           destFile->close();
           delete destFile;
           destFile = nullptr;
           //Sleep(500);

           ui->recvProg->setValue(0); // 进度条归零
           fileBytes = nameSize = gotBytes = 0;
//         delete receive;
//           receive = nullptr;
           delete file;
           file = nullptr;
           fileName.clear();

           if(!server->isListening()){
               quint16 port = ui->portLineEdit->text().toUShort();
               server->listen(QHostAddress::Any, port);
           }

       }
    } catch (QException *e) {
        ui->StatusText->appendPlainText(QString("%1 %2").arg(get_time()).arg(e->what()));
        log->appendLog(e->what());
        receive->close(); // 关socket
        receive = Q_NULLPTR;
        file = Q_NULLPTR;
        destFile = nullptr;
        fileName.clear(); // 清空文件名
        fileBytes = gotBytes = nameSize = 0;
        ui->recvProg->reset(); // 进度条归零
        ui->listenBtn->setEnabled(true); // 启用监听按钮
    } catch (std::exception *e) {
        ui->StatusText->appendPlainText(QString("%1 %2").arg(get_time()).arg(e->what()));
        log->appendLog(e->what());
        receive->close(); // 关socket
        receive = Q_NULLPTR;
        file = Q_NULLPTR;
        destFile = nullptr;
        fileName.clear(); // 清空文件名
        fileBytes = gotBytes = nameSize = 0;
        ui->recvProg->reset(); // 进度条归零
        ui->listenBtn->setEnabled(true); // 启用监听按钮
    }
}

void ReceiveFile::show_error(QAbstractSocket::SocketError)
{
    //std::cerr << "*** Socket Error ***" << std::endl;
    ui->StatusText->appendPlainText(QString("%1 %2").arg(get_time()).arg(receive->errorString()));
    log->appendLog(receive->errorString());
    receive->close(); // 关socket
    receive = Q_NULLPTR;
    file = Q_NULLPTR;
    destFile = nullptr;
    fileName.clear(); // 清空文件名
    fileBytes = gotBytes = nameSize = 0;
    ui->recvProg->reset(); // 进度条归零
    //ui->listenBtn->setEnabled(true); // 启用监听按钮
    ui->StatusText->appendPlainText(QString("%1 *** SOCKET ERROR").arg(get_time()));
    if(!server->isListening()){
        quint16 port = ui->portLineEdit->text().toUShort();
        server->listen(QHostAddress::Any, port);
    }
}


void ReceiveFile::on_listenBtn_clicked()
{
    /* 禁用监听按钮 */
    ui->listenBtn->setEnabled(false);
    ui->recvProg->setValue(0);
    QString port = ui->portLineEdit->text();
    if(port.size() == 0 || port == nullptr) {
        QString dlgTitle = "消息提示框";
        QString strInfo = "请检查端口号是否填写";
        QMessageBox::information(this, dlgTitle, strInfo,QMessageBox::Ok,QMessageBox::NoButton);
        return;     //直接返回，重新填写端口号
    }
    if(!server->listen(QHostAddress::Any, port.toUShort()))
    {
        ui->StatusText->appendPlainText(QString("%1 *** Listen to Port Failed ***").arg(get_time()));
        log->appendLog("*** Listen to Port Failed ***");
        ui->StatusText->appendPlainText(QString("%1 ").arg(server->errorString()));
        log->appendLog(server->errorString());
        server->close();
        ui->listenBtn->setEnabled(true);
        return;
    }
    ui->StatusText->appendPlainText(QString("%1 Listening to Port %2").arg(get_time()).arg(port));
    log->appendLog(QString("Listening to Port %1").arg(port));
}



void ReceiveFile::on_selectDirBtn_clicked()
{
     directory = QFileDialog::getExistingDirectory(this,"请选择接收文件的文件夹","",
                                                   QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
     if (!directory.isEmpty()) {
         if (ui->directoryComboBox->findText(directory) == -1)
             ui->directoryComboBox->addItem(directory);
         ui->directoryComboBox->setCurrentIndex(ui->directoryComboBox->findText(directory));
     }
}
