#include "rmainwindow.h"
#include "ui_rmainwindow.h"
#include <QDir>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <winuser.h>
#include <exception>
#include <QException>

rMainWindow::rMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::rMainWindow)
{
    this->mthread = new myThread();
    this->mr = new mergeThread();
    this->mt = new MergeTimeThread();
    log = new Logger();
    ui->setupUi(this);
    connect(this->mthread, SIGNAL(SignalUpdateText(QString)),this, SLOT(slotUpdateText(QString)));
    connect(this->mt->getThread(), SIGNAL(SignalUpdateText(QString)),this, SLOT(slotUpdateText(QString)));
    connect(this->mt, SIGNAL(updateText(QString)), this, SLOT(slotUpdateText(QString)));
    connect(this->mr, SIGNAL(updateText(QString)), this, SLOT(slotUpdateText(QString)));
    ui->pBtnStart->setEnabled(false);
    ui->autoDaysEdit->setAlignment(Qt::AlignHCenter);
    // QString::fromLocal8Bit()，QStringLiteral()防止中文乱码
    this->setWindowTitle(QStringLiteral("数据库恢复"));
    this->setFixedSize(this->width(),this->height());
}

rMainWindow::~rMainWindow()
{
    if (this->mthread)
        delete this->mthread;
    delete ui;
}


void rMainWindow::on_toolButton_clicked()
{
    ui->pBtnStart->setEnabled(true);
    ui->FileLists->clear();
    QString strs;
    QStringList file_list, output_name;
    QStringList str_path_list = QFileDialog::getOpenFileNames(this, QStringLiteral("选择要合并成数据库的文件"));
    for (int i = 0; i < str_path_list.size(); ++i) {
        QString str_path = str_path_list[i];
        if (!str_path.isEmpty()){
            ui->FileLists->addItem(str_path);
           // ui->FileLists->setCurrentRow(i);
        }
    }
}

std::string get_time()
{
    const time_t t = time(nullptr);
    tm* local; //本地时间
    char buf[128]= {0};
    local = localtime(&t); //转为本地时间
    strftime(buf, 64, "%Y%m%d", local);
    std::string res = buf;
    return res;
}

//按照大小合并数据库备份文件
void rMainWindow::mergeDBbysize()
{
    try {
        std::vector<std::string> file_names;
        QString file_name;
        for (int i = 0; i < ui->FileLists->count(); ++i) {
            file_name = ui->FileLists->item(i)->text();
            file_names.emplace_back(file_name.toStdString());
            //std::cout << file_name.toStdString() << std::endl;
        }
        std::sort(file_names.begin(), file_names.end());
        for (size_t i = 0; i < file_names.size(); i++) {
            slotUpdateText(QString::fromStdString(file_names[i]));
            this->repaint();
        }
        std::string command = "gbak -replace -user sysdba -password masterkey ";

        // 拼接文件名组成命令
        for (size_t i = 0; i < file_names.size(); i++) {
            command += file_names[i];
            command += " ";
            //ui->plainTextEdit->appendPlainText(QString::fromStdString(command));
            this->repaint();
        }
        std::string dbPath = file_names[0];
        std::string::size_type pos = dbPath.find("_");
        dbPath.replace(dbPath.begin() + int(pos) + 1, dbPath.end(), get_time());
        dbPath.append(".FDB");
        command.append(dbPath);
        slotUpdateText("数据库文件的路径：" + QString::fromStdString(dbPath));
        log->appendLog(QString("数据库文件的路径：%1").arg(dbPath.c_str()));
        this->repaint();

        this->mthread->setCommand(command);
        this->mthread->start();

    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e){
        log->appendLog(e->what());
    }

}

void rMainWindow::mergeDBbyTime()
{
    try {
        std::vector<std::string> file_names;
        QString file_name;

        QFile nbk_state(directory + "/" + "nbk_state.txt");
        while (!nbk_state.exists());    //如果文件不存在，则停止
        std::set<std::string> result = read_file("nbk_out.txt");

        for(auto i=result.begin(); i != result.end(); i++) {
            file_names.emplace_back(*i);
            std::cout << *i << std::endl;
        }
        //std::sort(file_names.begin(), file_names.end());

        std::string command = "nbackup -R ";
        std::string temp_file = file_names[file_names.size() - 1];      //temp_file最终为dbPath数据库名绝对路径
        std::string::size_type pos_1 = temp_file.find('_');
        std::string::size_type pos_2 = temp_file.find('.');
        if ( pos_2 == std::string::npos ) {
            slotUpdateText(QString::fromStdString("未找到."));
            this->repaint();
            return;
        }

        temp_file.replace(temp_file.begin() + int(pos_1) + 1, temp_file.end(), "");
        temp_file.append(get_time());
        temp_file.append(".FDB");

        command += temp_file;
        for (size_t i = 0; i < file_names.size(); i++) {
            command += " ";
            command += file_names[i];
            //emit this->mthread->SignalUpdateText(QString::fromStdString(command));
        }
        slotUpdateText("数据库文件的路径：" + QString::fromStdString(temp_file));
        log->appendLog(QString("数据库文件的路径：%1").arg(temp_file.c_str()));
        this->mthread->setCommand(command);
        this->mthread->start();
    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }

}

std::set<std::string> rMainWindow::read_file(std::string m_file)
{

    QString file_name = directory + "/" + m_file.c_str();

    QFile new_file(file_name);
    QStringList file_list;
    if (new_file.open(QIODevice::ReadOnly)) {
        while (!new_file.atEnd()) {
            QByteArray line = new_file.readLine();
            QString temp_name(line);
            file_list.append(temp_name);
        }
    }
    std::set<std::string> result;
    for (int i = 0; i < file_list.size(); i++) {
        file_list[i].remove("\n");
        result.insert(file_list[i].toStdString());
    }
    return result;
}

void rMainWindow::slotUpdateText(QString str)
{
    QString now = getTime();
    ui->plainTextEdit->appendPlainText(QString("%1 %2").arg(now).arg(str));
}

void rMainWindow::on_pBtnStart_clicked()
{
    if (ui->rBtnSize->isChecked()) {
        mergeDBbysize();
    }
    else if (ui->rBtnTime->isChecked()) {
        mergeDBbyTime();
    }
    else {
        mergeDBbyTime();
    }
}

QString rMainWindow::getTime()
{
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss :");
    return current_date_time;
}

void rMainWindow::on_selectDirBtn_clicked()
{
    directory = QFileDialog::getExistingDirectory(this, "选择要恢复文件所在的文件夹", "");
    if (!directory.isEmpty()) {
       if (ui->directoryComboBox->findText(directory) == -1) {
           ui->directoryComboBox->addItem(directory);
       }
       ui->directoryComboBox->setCurrentIndex(ui->directoryComboBox->findText(directory));
    }
}

void rMainWindow::on_pBtnExit_clicked()
{
    if (directory.isEmpty()) {
        QString dlgTitle = "消息提示框";
        QString strInfo = "恢复文件夹不能为空，请重新选择";
        QMessageBox::information(this, dlgTitle, strInfo,QMessageBox::Ok,QMessageBox::NoButton);
        return;     //直接返回，重新选择数据库文件
    }
    ui->pBtnExit->setEnabled(false);    //禁用
    slotUpdateText("等待恢复中...");
    this->mr->setDir(directory);
    this->mr->start();      //合并00整个备份文件

    QString QStr_days = ui->autoDaysEdit->text();
    if (QStr_days.isEmpty() ||QStr_days == "") {
        this->mt->setDays(1);
    }
    unsigned long days = QStr_days.toUInt();

    this->mt->setDays(days);

    this->mt->setDirectory(directory);
    this->mt->start();      //合并恢复成整个数据库文件

}
