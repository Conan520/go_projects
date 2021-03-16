#include "mergeThread.h"

mergeThread::mergeThread()
{
    log = new Logger();
}

mergeThread::~mergeThread()
{
    if (log)
        delete log;
}

void mergeThread::setDir(QString dir)
{
    this->directory = dir;
}

QStringList mergeThread::read_file(std::string m_file)
{
    try {
        QFile state_file(directory + "/" + "nbk_00_state.txt");
        while (!state_file.exists());   //如果不存在则表示开始的几个文件还没有传输完成
        QString file_name = directory + "/" + m_file.c_str();
    //    std::cout << file_name.toStdString() << std::endl;
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
        }
        file_list.removeDuplicates();
        file_list.sort();   //排序，一般情况已经是排好序的
        return file_list;
    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }
}

//合并第一次备份的文件
void mergeThread::mergeFile()
{
    try {
        QStringList fileList = read_file("nbk_00_out.txt");
        std::string fileName = fileList[0].toStdString();
        std::string::size_type pos = fileName.rfind("_");
        fileName.erase(pos, 3);
    //    std::cout << fileName << std::endl;
        emit updateText(QString("正在合并 %1").arg(fileName.c_str()));
        log->appendLog(QString("正在合并 %1").arg(fileName.c_str()));
        QFile file_name(fileName.c_str());
        if (!file_name.open(QIODevice::WriteOnly)) {
            emit updateText("file is opened failed!");
        }
        for (int i = 0; i < fileList.size(); i++) {
            QFile temp_file(fileList[i]);
            qint64 fileSize = temp_file.size();
            char* buff = new char[fileSize];
            memset(buff, 0, fileSize);
            qint64 read_bytes = 0;
            if (temp_file.open(QIODevice::ReadOnly)) {

                read_bytes = temp_file.read(buff, fileSize);
                while(read_bytes < fileSize) {
                    read_bytes += temp_file.read(buff + read_bytes, fileSize - read_bytes);
                }
                qint64 write_bytes = file_name.write(buff, read_bytes);
                while(write_bytes < read_bytes) {
                    write_bytes += file_name.write(buff + write_bytes, read_bytes - write_bytes);
                }
            }
            memset(buff, 0, fileSize);
            temp_file.close();
            delete[] buff;
        }
        file_name.close();
        emit updateText(QString("%1 已经合并成功").arg(fileName.c_str()));
        log->appendLog(QString("%1 已经合并成功").arg(fileName.c_str()));
        //合并好的文件名存入文件，方便恢复
        QFile nbk_file(directory + "/" + "nbk_out.txt");
        if (nbk_file.exists() && nbk_file.open(QIODevice::WriteOnly |QIODevice::Append)) {
            nbk_file.write(fileName.c_str());
            nbk_file.write("\n");
            nbk_file.close();
        }
        QFile nbk_state(directory + "/" + "nbk_state.txt");
        if (nbk_state.open(QIODevice::WriteOnly)) {
            nbk_state.write("1");
            nbk_state.close();
        }
    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }
}

void mergeThread::run()
{
    mergeFile();
}
