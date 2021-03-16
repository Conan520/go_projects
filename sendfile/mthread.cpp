#include "mthread.h"

mThread::mThread()
{
    send = nullptr;
    log = new Logger();
}

mThread::~mThread()
{
    if (log)
        delete log;
}

void mThread::setFolder(QString folder)
{
    this->folder = folder;
    tcp_state = 1;
}

QStringList mThread::read_file(std::string file)
{
    //QString state_file = folder + "/" + "gbak_state.txt";
    QString state_file = folder + "/" + "split_00_state.txt";
    while (!QFile(state_file).exists()){
    }
    QString file_name = folder + "/" + file.c_str();
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

void mThread::setTcpSocket(QTcpSocket *send)
{
    this->send = send;
}

void mThread::setState(int state)
{
    this->tcp_state = state;
}

void mThread::run()
{
    try {
        QStringList file_list = read_file("nbk_split.txt");
        int len = file_list.size();
        for (int i = 0; i < len; i++) {
            if (!fileName.isEmpty()) {
                fileName.clear();
            }
            fileName = file_list[i];
            emit setfile(fileName);
            emit send_file();
            msleep(100);    //睡眠100ms
            while(send==nullptr);
            while(send->state() == 0 || send->state() == 2);
//            std::cout << fileName.toStdString() << (unsigned long long)send << std::endl;
//            std::cout << "state: " << send->state() << std::endl;
//            std::cout.flush();
            while(send->state() != 0);

        }
        while(true) {
            QFile stateFile(folder + "/" + "nbk_state.txt");
            while(!stateFile.exists());
            if (stateFile.open(QIODevice::ReadOnly)) {
                fileName = stateFile.readAll();
            }
            stateFile.remove();     //确认状态文件存在之后，删除文件以便判断下次
            emit setfile(fileName);
            emit send_file();
            msleep(100);
            while(!send);

            while(send->state() == 0 || send->state() == 2);
//            std::cout << fileName.toStdString() << (unsigned long long)send << std::endl;
//            std::cout << "state: " << send->state() << std::endl;

            while(send->state() != 0);
        }
    } catch (QException *e) {
        emit updateText(e->what());
        log->appendLog(e->what());
    } catch (std::exception *e) {
        emit updateText(e->what());
        log->appendLog(e->what());
    }
}
