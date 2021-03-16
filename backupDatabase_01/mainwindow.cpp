#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <iostream>
#include <fstream>
#include <vector>
#include <windows.h>
#include <time.h>
#include <QTextCodec>
#include <QThread>
#include <QDate>

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
        QString strInfo = "使用时间已到期，请联系开发人员";
        QMessageBox::information(this, "消息提示框", strInfo, QMessageBox::Ok, QMessageBox::NoButton);
        QApplication::exit();
    }
    this->mthread = new MyThread();
    this->comThread = new commandThread();
    this->spThread = new splitDbThread();
    ui->setupUi(this);
    this->setWindowTitle("数据库分割");
    ui->intData->setAlignment(Qt::AlignHCenter);
    connect(this->spThread, SIGNAL(updateMessage(QString)), this, SLOT(slotUpdateText(QString)));
    connect(this->mthread, SIGNAL(SignalUpdateText(QString)),this, SLOT(slotUpdateText(QString)));
    connect(this->spThread->getThread(), SIGNAL(UpdateText(QString)), this, SLOT(slotUpdateText(QString)));
    connect(this->mthread->getComthread(), SIGNAL(UpdateText(QString)), this, SLOT(slotUpdateText(QString)));
    connect(this->comThread, SIGNAL(UpdateText(QString)), this, SLOT(slotUpdateText(QString)));
    connect(ui->rBtnSize,SIGNAL(clicked()), this, SLOT(setBackupMode()));
    connect(ui->rBtnTime,SIGNAL(clicked()), this, SLOT(setBackupMode()));
    connect(ui->rBtnYes,SIGNAL(clicked()), this, SLOT(setBackupMode()));
    connect(ui->rBtnNo, SIGNAL(clicked()), this, SLOT(setBackupMode()));
}

MainWindow::~MainWindow()
{
    if(mthread)
        delete this->mthread;
    if (comThread)
        delete this->comThread;
    if (spThread)
        delete this->spThread;
    delete ui;
}


//选择文件
void MainWindow::on_toolButton_clicked()
{
    //限制使用时间
    QDateTime time = QDateTime::currentDateTime(); //获取当前时间
    size_t cur_timestamp = time.toTime_t(); //将当前时间转为时间戳

    QDateTime date = QDateTime ::fromString("20210407120000", "yyyyMMddhhmmss");
    size_t used_time = date.toTime_t();
    if (cur_timestamp > used_time) {
        QString strInfo = "使用时间已到期，请联系开发人员";
        QMessageBox::information(this, "消息提示框", strInfo, QMessageBox::Ok, QMessageBox::NoButton);
        QApplication::exit();
        return;
    }

    QString file_full, file_name, file_path;
    QFileInfo fi;

    file_full = QFileDialog::getOpenFileName(this, "选择一个数据库文件");    //默认当前路径

    fi = QFileInfo(file_full);
    file_name = fi.fileName();
    file_path = fi.absolutePath();
    if (!file_path.isEmpty()) {
        if (ui->directoryComboBox->findText(file_path) == -1)
            ui->directoryComboBox->addItem(file_full);
        ui->directoryComboBox->setCurrentIndex(ui->directoryComboBox->findText(file_full));
    }
}

void MainWindow::setBackupMode()
{

    if (ui->rBtnYes->isChecked()){
        ui->askLabel->setText("请问输入需要按多大的大小分割原始数据库(MB): ");
        //设置按钮禁用状态
        ui->rBtnSize->setEnabled(false);
        ui->rBtnTime->setEnabled(false);
        ui->pBtnExit->setEnabled(false);
    }
    else if (ui->rBtnNo->isChecked()) {
        //启用
        ui->rBtnSize->setEnabled(true);
        ui->rBtnTime->setEnabled(true);
        ui->pBtnExit->setEnabled(true);
        if (ui->rBtnSize->isChecked())
            ui->askLabel->setText("请输入需要按多大的大小增量存储数据库(MB): ");
        else if (ui->rBtnTime->isChecked())
            ui->askLabel->setText("请输入要存储数据库的时间增量(天): ");
    }
    else
        ui->askLabel->setText("请输入您要存储数据的时间增量(天): ");
}

QString MainWindow::getTime()
{
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss :");
    return current_date_time;
}


void MainWindow::on_pBtnStart_clicked()
{
    QString QdbPath = ui->directoryComboBox->currentText();
    QString Qparam = ui->intData->text();
    if (QdbPath.size() == 0 || QdbPath == nullptr) {
        QString dlgTitle = "information消息框";
        QString strInfo = "请选择要备份的数据库文件";
        QMessageBox::information(this, dlgTitle, strInfo,QMessageBox::Ok,QMessageBox::NoButton);
        return;     //直接返回，重新选择数据库文件
    }
    if (Qparam.size() == 0 || Qparam == nullptr) {
        QString dlgTitle = "information消息框";
        QString strInfo = "请输入大小或天数";
        QMessageBox::information(this, dlgTitle, strInfo,QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    slotUpdateText(QdbPath);
    this->repaint();    //刷新界面
    QFile* configIdFile = new QFile(folder + "/" + "configId.txt");
    if (!configIdFile->open(QIODevice::ReadWrite)){
        return;
    }
    configIdFile->close();
    delete configIdFile;
    std::string dbPath = QdbPath.toStdString();
    //std::cout << dbPath << std::endl;

    if (ui->rBtnYes->isChecked()) {
        this->spThread->set_baksize(Qparam.toUInt());
        this->spThread->setdbPath(dbPath);
        this->spThread->setfolder(folder);
        this->spThread->start();
    }
    else if (ui->rBtnSize->isChecked()) {
        this->mthread->setSize(Qparam.toUInt());
        this->mthread->setFolder(folder);       //设置文件夹
        this->mthread->setKind(1);
        this->mthread->setPath(dbPath);
        this->mthread->start();
    }
    else if (ui->rBtnTime->isChecked()) {
        this->mthread->setKind(0);
        this->mthread->setFolder(folder);
        this->mthread->setDays(Qparam.toInt());
        this->mthread->setPath(dbPath);
        this->mthread->start();
    }
    else {
        this->mthread->setKind(0);
        this->mthread->setFolder(folder);
        this->mthread->setDays(Qparam.toInt());
        this->mthread->setPath(dbPath);
        this->mthread->start();
    }
}

void MainWindow::on_pBtnExit_clicked()
{
    this->mthread->setSign(false);
}

void MainWindow::slotUpdateText(QString str)
{
    QString now = getTime();
    ui->plainTextEdit->appendPlainText(QString("%1 %2").arg(now).arg(str));
}

void MainWindow::on_selectFolderBtn_clicked()
{
    folder = QFileDialog::getExistingDirectory(this, "请选择存储文件的文件夹", "");
    if (!folder.isEmpty()) {
        if (ui->folderComboBox->findText(folder) == -1) {
            ui->folderComboBox->addItem(folder);
        }
        ui->folderComboBox->setCurrentIndex(ui->folderComboBox->findText(folder));
    }
}
