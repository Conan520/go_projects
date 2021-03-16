#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QByteArray>
#include <QDataStream>
#include <QFileDialog>
#include <QHostAddress>
#include <QIODevice>
#include <QString>
#include <QTcpSocket>
#include <QMessageBox>
#include <iostream>
#include <QDateTime>
#include <windows.h>

const quint16 PORT = 9030;
const qint64 LOADBYTES = 4 * 1024 * 1024; // 4 kilo-byte
const int DATA_STREAM_VERSION = QDataStream::Qt_5_12;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //限制使用时间
    QDateTime time = QDateTime::currentDateTime(); //获取当前时间
    size_t cur_timestamp = time.toTime_t(); //将当前时间转为时间戳

    QDateTime date = QDateTime ::fromString("20210407120000", "yyyyMMddhhmmss");
    size_t used_time = date.toTime_t();
    if (cur_timestamp > used_time) {
//        QString strInfo = "使用时间已到期，请联系开发人员";
//        QMessageBox::information(this, "消息提示框", strInfo, QMessageBox::Ok, QMessageBox::NoButton);
        QApplication::exit();
    }

    ui->setupUi(this);
    this->setWindowTitle("文件传输");
    ui->pTxtEdit->setReadOnly(true);
    //ui->ipLineEdit->setAlignment(Qt::AlignCenter);
    ui->portLineEdit->setAlignment(Qt::AlignHCenter);
    mthread = new mThread();
    send = new QTcpSocket(this);
    fileBytes = sentBytes = restBytes = 0;
    loadBytes = LOADBYTES;
    file = Q_NULLPTR;
    log = new Logger();
    ui->sendProg->setValue(0); // 进度条置零
    ui->sendBtn->setEnabled(false); // 禁用发送按钮
    //ui->cancelBtn->setEnabled(false);
    ui->portLineEdit->setText(QString::fromStdString(std::to_string(PORT)));
    connect(this->mthread, SIGNAL(setfile(QString)), this, SLOT(setFileName(QString)));
    connect(this->mthread, SIGNAL(send_file()), this, SLOT(on_sendBtn_clicked()));
    connect(this->mthread, SIGNAL(updateText(QString)), this, SLOT(slotUpdatetext(QString)));
    connect(ui->sendBtn, SIGNAL(clicked()), this, SLOT(on_sendBtn_clicked()));

    /* 连接已建立 -> 开始发数据 */
    connect(send, SIGNAL(connected()),
            this, SLOT(start_transfer()));      //connectToHost()调用成功后发送connected()信号
    /* 数据已发出 -> 继续发 */
    connect(send, SIGNAL(bytesWritten(qint64)),
            this, SLOT(continue_transfer(qint64)));
    /* socket出错 -> 错误处理 */
    connect(send, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(show_error(QAbstractSocket::SocketError)));
}


MainWindow::~MainWindow()
{
    delete ui;
    delete send;
    delete log;
}

QString MainWindow::get_time()
{
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss :");
    return current_date_time;
}

void MainWindow::slotUpdatetext(QString str)
{
    QString now = get_time();
    ui->pTxtEdit->appendPlainText(QString("%1 %2").arg(now).arg(str));
}

QStringList MainWindow::read_file(std::string backupFile)
{
    QString state_file = folder + "/" + "gbak_state.txt";
    while (!QFile(state_file).exists()){
    }
    QString file_name = folder + "/" + backupFile.c_str();
    QFile new_file(file_name);
    QStringList file_list;
    if (new_file.open(QIODevice::ReadOnly)) {
        while (!new_file.atEnd()) {
            QByteArray line = new_file.readLine();
            QString temp_name(line);
            file_list.append(temp_name);
        }
    }

    for (int i = 0; i < file_list.size(); i++) {
        file_list[i].remove("\n");
//        std::cout << file_list[i].toStdString() << std::endl;
    }
    return file_list;
}

void MainWindow::start_transfer()
{
    try {
        file = new QFile(fileName);
        if(!file->open(QIODevice::ReadOnly))
        {
            ui->pTxtEdit->appendPlainText(QString("%1 *** FILE OPEN ERROR").arg(get_time()));
            ui->pTxtEdit->appendPlainText(QString(" %1 *** start_transfer(): File %2-Open-Error").arg(get_time()).arg(fileName));
            log->appendLog(QString("%1 open failed!").arg(fileName));
            return;
        }

        fileBytes = file->size();
        ui->sendProg->setValue(0);
        ui->pTxtEdit->appendPlainText(QString("%1 Connection Established!").arg(get_time()));
        log->appendLog("Connection Established ！");
        ui->pTxtEdit->appendPlainText(QString("%1 In sending %2").arg(get_time()).arg(fileName));
        log->appendLog(QString("In sending %1").arg(fileName));
        QByteArray buf;
        QDataStream out(&buf, QIODevice::WriteOnly);    //创建数据流来操作buf
        out.setVersion(DATA_STREAM_VERSION);

        /* 无路径文件名 */
        QString sfName = fileName.right(fileName.size() - fileName.lastIndexOf('/') - 1);
        /* 首部 = 总大小 + 文件名长度 + 文件名 */
        out << qint64(0) << qint64(0) << sfName;
        /* 总大小加上首部的大小 */
        fileBytes += buf.size();
        ui->sendProg->setMaximum(fileBytes);
        /* 重写首部的前两个长度字段 */
        out.device()->seek(0);
        out << fileBytes << (qint64(buf.size()) - 2 * sizeof(qint64));
        /* 发送首部，计算剩余大小 */
        restBytes = fileBytes - send->write(buf);

    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }
}

void MainWindow::continue_transfer(qint64 sentSize)
{
    try {
        sentBytes += sentSize;
        ui->sendProg->setValue((sentBytes));
        this->repaint();
        /* 还有数据要发 */
        if(restBytes > 0)
        {
            /* 从文件读数据 */
            QByteArray buf = file->read(qMin(loadBytes, restBytes));
            /* 发送 */
            restBytes -= send->write(buf);
        }
        else
            file->close();
        /* 全部发送完 */
        if(sentBytes == fileBytes)
        {
            send->close(); // 关socket
//            std::cout <<  "**state:" << send->state() << std::endl;
            ui->pTxtEdit->appendPlainText(QString("%1 Finish sending %2").arg(get_time()).arg(fileName));
            log->appendLog(QString("Finish sending %1").arg(fileName));
            fileName.clear(); // 清空文件名
            ui->sendProg->setValue(0); // 进度条归零
        }
    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }
}

void MainWindow::show_error(QAbstractSocket::SocketError)
{
    ui->pTxtEdit->appendPlainText(QString("%1 *** Socket Error").arg(get_time()));
    ui->pTxtEdit->appendPlainText(QString("%1 *** SOCKET ERROR, RESEND LATER").arg(get_time()));
    send->close();
    file = nullptr;
    ui->filePathBox->clear();
    ui->sendBtn->setEnabled(true);
    ui->autoBtn->setEnabled(true);
    ui->sendProg->reset(); // 进度条归零
    fileName.clear();
}


void MainWindow::on_selectBtn_clicked()
{
    //限制使用时间
    QDateTime time = QDateTime::currentDateTime(); //获取当前时间
    size_t cur_timestamp = time.toTime_t(); //将当前时间转为时间戳

    QDateTime date = QDateTime ::fromString("20210407120000", "yyyyMMddhhmmss");
    size_t used_time = date.toTime_t();
    if (cur_timestamp > used_time) {
//        QString strInfo = "使用时间已到期，请联系开发人员";
//        QMessageBox::information(this, "消息提示框", strInfo, QMessageBox::Ok, QMessageBox::NoButton);
        QApplication::exit();
        return;     //直接返回
    }

    /* 开文件选择窗选文件，返回带路径文件名 */
    fileName = QFileDialog::getOpenFileName(this, "选择一个要传输的文件");

    if(!fileName.isEmpty())
    {
        ui->pTxtEdit->appendPlainText(QString("%1 File %2 Opened!").arg(get_time()).arg(fileName));
        ui->sendBtn->setEnabled(true);
        if (ui->filePathBox->findText(fileName) == -1)
            ui->filePathBox->addItem(fileName);
        ui->filePathBox->setCurrentIndex(ui->filePathBox->findText(fileName));
    }
    else
        ui->pTxtEdit->appendPlainText(QString("%1 *** FAIL OPENING FILE").arg(get_time()));
}

void MainWindow::on_sendBtn_clicked()
{
    try {
        QString ip_str = ui->ipLineEdit->text();
        QHostAddress ip;
        bool is_Addr = ip.setAddress(ip_str);
        if (is_Addr)
            ip = QHostAddress(ip_str);
        else {
            QString strInfo = "ip地址有误，请重新输入";
            QMessageBox::information(this, "消息提示框", strInfo,QMessageBox::Ok,QMessageBox::NoButton);
            return;     //直接返回，重新填写ip地址
        }
        QString port = ui->portLineEdit->text();
        if (ip_str.size() == 0 || ip_str == nullptr) {
            QString dlgTitle = "消息提示框";
            QString strInfo = "请检查ip地址是否填写";
            QMessageBox::information(this, dlgTitle, strInfo,QMessageBox::Ok,QMessageBox::NoButton);
            return;     //直接返回，重新填写ip地址
        }
        if (port.size() == 0 || port == nullptr) {
            QString dlgTitle = "消息提示框";
            QString strInfo = "请检查端口号是否填写";
            QMessageBox::information(this, dlgTitle, strInfo,QMessageBox::Ok,QMessageBox::NoButton);
            return;     //直接返回，重新填写端口号
        }

        /* 发送连接请求 */
        send->connectToHost(ip, port.toUShort());
        this->mthread->setTcpSocket(send);

        sentBytes = 0;
        ui->sendBtn->setEnabled(false);
        //ui->cancelBtn->setEnabled(true);
        ui->pTxtEdit->appendPlainText(QString("%1 Linking...").arg(get_time()));
        log->appendLog("Linking...");
    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }


}

void MainWindow::setFileName(QString fileName)
{
    this->fileName = fileName;
}

void MainWindow::on_selectFolderBtn_clicked()
{
    folder = QFileDialog::getExistingDirectory(this, "选择要传输文件所在的文件夹", "");
    if (!folder.isEmpty()) {
        if (ui->folderComboBox->findText(folder) == -1) {
            ui->folderComboBox->addItem(folder);
        }
        //ui->sendBtn->setEnabled(true);
        ui->folderComboBox->setCurrentIndex(ui->folderComboBox->findText(folder));
    }
}

void MainWindow::on_autoBtn_clicked()
{
    if (folder.isEmpty()) {
        QString dlgTitle = "消息提示框";
        QString strInfo = "传输文件夹不能为空，请重新选择";
        QMessageBox::information(this, dlgTitle, strInfo,QMessageBox::Ok,QMessageBox::NoButton);
        return;     //直接返回，重新选择数据库文件
    }
    /*检查ip地址是否合理*/
    QString ip_str = ui->ipLineEdit->text();
    QHostAddress ip;
    bool is_Addr = ip.setAddress(ip_str);
    if (is_Addr)
        ip = QHostAddress(ip_str);
    else {
        QString strInfo = "ip地址有误，请重新输入";
        QMessageBox::information(this, "消息提示框", strInfo,QMessageBox::Ok,QMessageBox::NoButton);
        return;     //直接返回，重新填写ip地址
    }

    ui->pTxtEdit->appendPlainText(QString("%1 Waiting for files to send !").arg(get_time()));
    ui->autoBtn->setEnabled(false);
    this->mthread->setFolder(folder);
    this->mthread->start();
}
