#include "mythread.h"


const int DATA_STREAM_VERSION = QDataStream::Qt_5_12;

MyThread::MyThread()
{
    this->comThread = new commandThread();
    this->log = new Logger();
}

MyThread::~MyThread()
{
    if (comThread)
        delete comThread;
}

size_t MyThread::GetFileSize(const std::string &file_name)
{
    std::ifstream in(file_name.c_str());
    in.seekg(0, std::ios::end);
    size_t size = in.tellg();
    in.close();
    return size; //单位是：byte
}

void MyThread::nbkFileSplit(const char *fileName)
{
    try {
        QFile destFile(folder + "/" + "nbk_split.txt");
        if (!destFile.open(QIODevice::WriteOnly))
            return;
        std::string temp_file = fileName;
        std::string::size_type pos = temp_file.find(".");
        temp_file.insert(pos, "_%02d");

        char temp[512] = {0};
        QFile *file = Q_NULLPTR;
        file = new QFile(fileName);
        if(!file->open(QFile::ReadOnly)){
            log->appendLog(QString::fromStdString(fileName));
            return;
        }
        qint64 splitSize = this->bak_size * 1024 * 1024;
        qint64 fileBytes = file->size();
        qint64 restBytes = fileBytes;   //剩余字节数
        qint64 readBytes = 0;   //已经读取的字节数
        QByteArray buf;
        long long fileNum = 0;
        QDataStream out(&buf, QIODevice::WriteOnly);    //创建数据流来操作buf
        out.setVersion(DATA_STREAM_VERSION);
        if (fileBytes < splitSize) {
            //std::cout << "文件过小，不宜分割" << std::endl;
            emit SignalUpdateText("文件过小，不宜分割");
            log->appendLog("文件过小，不宜分割");
            return ;
        }
        else {
            fileNum = fileBytes / splitSize + 1;
        }

        for (int i = 1; i <= fileNum; i++) {
            try {
                qint64 DATA_SIZE = qMin(splitSize, restBytes);
                char* data = new char[DATA_SIZE];
                memset(data, 0, DATA_SIZE);

                qint64 read_bytes = file->read(data, DATA_SIZE);

                while(read_bytes < DATA_SIZE) {
                    read_bytes += file->read(data + read_bytes, qMin(splitSize-read_bytes, restBytes));
                }
                readBytes += read_bytes;
                restBytes -= read_bytes;

                sprintf(temp, temp_file.c_str(), i);
                QFile newFile(temp);
                if (!newFile.open(QIODevice::WriteOnly)){
                    emit SignalUpdateText("file is opened failed !");
                    log->appendLog("file is opened failed !");
                    return;
                }
                destFile.write(temp);
                destFile.write("\n");
                destFile.flush();
                qint64 write_bytes = newFile.write(data, read_bytes);
                while (write_bytes < read_bytes) {
                    write_bytes += newFile.write(data + write_bytes, read_bytes - write_bytes);
                }
                newFile.close();
                memset(temp, 0, sizeof (temp));

                delete []data;
            } catch (QException *e) {
                emit SignalUpdateText(e->what());
                log->appendLog(e->what());
            }

        }
        destFile.close();
        file->close();
        QFile sign_file(folder + "/" + "split_00_state.txt");
        if (sign_file.open(QIODevice::WriteOnly)){
            sign_file.write("1");
        }
        sign_file.close();
    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }


}

//按照时间增量备份数据库
void MyThread::splitDatabaseByTime(std::string dbPath)
{
    try {
        time_t last_time = time(nullptr);
        int days = this->days;
        time_t std_time = days * 86400;
        //time_t std_time = 20;
        QFile destFile(folder + "/" + "nbak_out.txt");
        if (!destFile.open(QIODevice::WriteOnly)) {
            log->appendLog("nbak_out.txt open failed");
            return;
        }
        std::string command = "nbackup.exe -U sysdba -P masterkey -B ";

        QString QdbPath = dbPath.c_str();   //转为QString，方便取文件名
        QString file_name = QdbPath.right(QdbPath.size() - QdbPath.lastIndexOf('/') - 1);    //只取文件名
        file_name = folder + "/" + file_name;     //合并成新的绝对路径

        std::string tmp_name = file_name.toStdString();
        std::string::size_type pos = tmp_name.find(".");
        if (pos == std::string::npos) {
            emit SignalUpdateText(QString::fromStdString("未找到."));
            log->appendLog("未找到.");
            return ;
        }
        tmp_name.insert(pos, "_%02d");
        char c = ' ';
        tmp_name.insert(tmp_name.begin(), c);
        pos = tmp_name.find(".");
        tmp_name.replace(pos + 1, pos + 4, "nbk");  //将FDB替换为nbk

        char temp[512] = {0};   //备份文件的绝对路径名
        int id = 0;
        std::string fileName = "";
        QFile configIdFile(folder + "/" + "configId.txt");
        if (configIdFile.open(QIODevice::ReadOnly)) {
            QString content = configIdFile.readAll();
            if (!content.isEmpty()) {
                id = content.toInt();
            }
            configIdFile.close();
        }
        if (id ==  0){
            std::string init_str;
            sprintf(temp, tmp_name.c_str(), id);
            init_str.append(std::to_string(id) + " " + dbPath + temp);
            std::string end_command = command + init_str;
            fileName = temp;
            fileName.erase(0, 1);   //去掉最开始的空格
            emit SignalUpdateText(QString::fromStdString(fileName));
            log->appendLog(QString::fromStdString(fileName));
            this->comThread->setCommand(end_command);
            this->comThread->start();   //运行命令
            while(!this->comThread->isFinished());
            destFile.write(fileName.c_str());
            destFile.write("\n");
            destFile.flush();
            nbkFileSplit(fileName.c_str());
        }

        while(this->sign) {
            std::string str = "";	//用来存放后面的字符串，例如：0 D:/dbtest.FDB D:/dbtest_00.FDB
            time_t cur_time = time(nullptr);
            if (cur_time - last_time >= std_time) {
                ++id;
                last_time = cur_time;
                sprintf(temp, tmp_name.c_str(), id);
                fileName = temp;
                fileName.erase(0, 1);
                destFile.write(fileName.c_str());	//将文件名保存到文件中
                destFile.write("\n");
                destFile.flush();
                str.append(std::to_string(id) + " " + dbPath + temp);
                emit SignalUpdateText(QString::fromStdString(fileName));
                log->appendLog(QString::fromStdString(fileName));
                memset(temp, 0, sizeof (temp));
                std::string final_command = command + str;
                this->comThread->setCommand(final_command);
                this->comThread->start();
                while(!this->comThread->isFinished());
                if (!configIdFile.open(QIODevice::WriteOnly)) {
                    emit SignalUpdateText("The confifId.txt is not opened normally");
                    log->appendLog("The confifId.txt is not opened normally");
                    return;
                }
                configIdFile.write(std::to_string(id).c_str());
                configIdFile.flush();
                configIdFile.close();
                QFile temp_state_file(folder + "/" + "nbk_state.txt");
                if(temp_state_file.open(QIODevice::WriteOnly)) {
                    temp_state_file.write(fileName.c_str());
                    temp_state_file.close();
                }
                fileName.clear();
            }
            //限制使用时间
            QDateTime time = QDateTime::currentDateTime(); //获取当前时间
            size_t cur_timestamp = time.toTime_t(); //将当前时间转为时间戳

            QDateTime date = QDateTime ::fromString("20210407120000", "yyyyMMddhhmmss");
            size_t used_time = date.toTime_t();
            if (cur_timestamp > used_time) {
                this->comThread->exit();
                QApplication::exit();
                return;
            }
            Sleep(2*1000);	//暂停
        }
        std::string str;
        ++id;
        sprintf(temp, tmp_name.c_str(), id);
        fileName = temp;
        fileName.erase(0, 1);
        destFile.write(fileName.c_str());		//将文件名保存到文件中，方便以后的合并
        destFile.flush();
        destFile.close();
        emit SignalUpdateText(QString::fromStdString(fileName));
        log->appendLog(QString::fromStdString(fileName));
        str.append(std::to_string(id) + " " + dbPath + temp);
        std::string final_command = command + str;
        emit SignalUpdateText(QString::fromStdString("正在进行最后的存储"));
        log->appendLog("正在进行最后的存储");
        this->comThread->setCommand(final_command);
        this->comThread->start();
        while(!this->comThread->isFinished());
        if (!configIdFile.open(QIODevice::WriteOnly)) {
            emit SignalUpdateText("The confifId.txt is not opened normally ");
            log->appendLog("The confifId.txt is not opened normally ");
            return;
        }
        configIdFile.write(std::to_string(id).c_str());
        configIdFile.flush();
        configIdFile.close();
        QFile temp_state_file(folder + "/" + "nbk_state.txt");
        if(temp_state_file.open(QIODevice::WriteOnly)) {
            temp_state_file.write(fileName.c_str());
            temp_state_file.close();
        }
        fileName.clear();
        setSign(true);
        emit SignalUpdateText(QString::fromStdString("存储任务已停止···"));
        log->appendLog("存储任务已停止···");
    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }
}

//按照大小增量备份数据库
void MyThread::splitDatabaseBySize(std::string dbPath)
{
    try {
        size_t bak_size = this->bak_size * 1024 * 1024;
        qint64 last_size = QFile(dbPath.c_str()).size();
        QFile destFile(folder + "/" + "nbak_out.txt");
        if (!destFile.open(QIODevice::WriteOnly)){
            log->appendLog("nbak_out.txt open failed !");
            return;
        }

        std::string command = "nbackup.exe -U sysdba -P masterkey -B ";
        QString QdbPath = dbPath.c_str();   //转为QString，方便取文件名
        QString file_name = QdbPath.right(QdbPath.size() - QdbPath.lastIndexOf('/') - 1);    //只取文件名
        file_name = folder + "/" + file_name;     //合并成新的绝对路径
        std::string tmp_name = file_name.toStdString();
        std::string::size_type pos = tmp_name.find(".");
        if (pos == std::string::npos) {
            emit SignalUpdateText(QString::fromStdString("未找到."));
            log->appendLog("未找到.");
            return;
        }
        tmp_name.insert(pos, "_%02d");
        char c = ' ';
        tmp_name.insert(tmp_name.begin(), c);
        pos = tmp_name.find(".");
        tmp_name.replace(pos + 1, pos + 4, "nbk");

        char temp[512] = {0};
        std::string fileName = "";

        int id = 0;
        QFile configIdFile(folder + "/" + "configId.txt");
        if (configIdFile.open(QIODevice::ReadOnly)) {
            QString content = configIdFile.readAll();
            if (!content.isEmpty()) {
                id = content.toInt();
            }
            configIdFile.close();
        }
        if (id == 0) {
            std::string init_str;
            sprintf(temp, tmp_name.c_str(), id);
            init_str.append(std::to_string(id) + " " + dbPath + temp);
            std::string end_command = command + init_str;

            fileName = temp;
            fileName.erase(0, 1);
            emit SignalUpdateText(QString::fromStdString(fileName));
            log->appendLog(QString::fromStdString(fileName));
            this->comThread->setCommand(end_command);
            this->comThread->start();   //运行命令
            while(!this->comThread->isFinished());

            destFile.write(fileName.c_str());
            destFile.write("\n");
            destFile.flush();
            nbkFileSplit(fileName.c_str());
        }

        while(this->sign) {
            std::string str;	//用来存放后面的字符串，例如：0 D:/dbtest.FDB D:/dbtest_00.FDB
            qint64 current_size = QFile(dbPath.c_str()).size();
            if (current_size - last_size > bak_size) {
                ++id;
                last_size = current_size;
                sprintf(temp, tmp_name.c_str(), id);
                fileName = temp;
                fileName.erase(0, 1);
                destFile.write(fileName.c_str());		//将文件名保存到文件中，方便以后的合并
                destFile.write("\n");
                destFile.flush();
                emit SignalUpdateText(QString::fromStdString(fileName));
                log->appendLog(QString::fromStdString(fileName));
                str.append(std::to_string(id) + " " + dbPath + temp);
                std::string final_command = command + str;
                memset(temp, 0, sizeof(temp));

                this->comThread->setCommand(final_command);
                this->comThread->start();
                while(!this->comThread->isFinished());

                if (!configIdFile.open(QIODevice::WriteOnly)) {
                    emit SignalUpdateText("The confifId.txt is not opened normally ");
                    log->appendLog("The confifId.txt is not opened normally");
                    return;
                }
                configIdFile.write(std::to_string(id).c_str());
                configIdFile.flush();
                configIdFile.close();
                QFile temp_state_file(folder + "/" + "nbk_state.txt");
                if(temp_state_file.open(QIODevice::WriteOnly)) {
                    temp_state_file.write(fileName.c_str());
                    temp_state_file.close();
                }
                fileName.clear();
            }
            //限制使用时间
            QDateTime time = QDateTime::currentDateTime(); //获取当前时间
            size_t cur_timestamp = time.toTime_t(); //将当前时间转为时间戳

            QDateTime date = QDateTime ::fromString("20210407120000", "yyyyMMddhhmmss");
            size_t used_time = date.toTime_t();
            if (cur_timestamp > used_time) {
                this->comThread->exit();
                QApplication::exit();
                return;
            }
            Sleep(2*1000);	//暂停
        }
        std::string str;
        ++id;
        sprintf(temp, tmp_name.c_str(), id);
        fileName = temp;
        fileName.erase(0, 1);
        destFile.write(fileName.c_str());		//将文件名保存到文件中，方便以后的合并
        destFile.write("\n");
        destFile.flush();
        destFile.close();
        emit SignalUpdateText(QString::fromStdString(fileName));
        str.append(std::to_string(id) + " " + dbPath + temp);
        std::string final_command = command + str;
        emit SignalUpdateText(QString::fromStdString("正在进行最后的存储"));
        log->appendLog(QString::fromStdString("正在进行最后的存储"));
        log->appendLog("正在进行最后的存储");
        this->comThread->setCommand(final_command);
        this->comThread->start();
        while(!this->comThread->isFinished());
        if (!configIdFile.open(QIODevice::WriteOnly)) {
            emit SignalUpdateText("The confifId.txt is not opened normally");
            log->appendLog("The confifId.txt is not opened normally");
            return;
        }
        configIdFile.write(std::to_string(id).c_str());
        configIdFile.flush();
        configIdFile.close();
        QFile temp_state_file(folder + "/" + "nbk_state.txt");
        if(temp_state_file.open(QIODevice::WriteOnly)) {
            temp_state_file.write(fileName.c_str());
            temp_state_file.close();
        }
        fileName.clear();
        emit SignalUpdateText(QString::fromStdString("存储任务已停止···"));
        log->appendLog("存储任务已停止···");
    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }

}

void MyThread::setSign(bool sign)
{
    this->sign = sign;
}

void MyThread::setDays(int days)
{
    this->days = days;
}

void MyThread::setPath(std::string dbPath)
{
    this->dbPath = dbPath;
}

void MyThread::setSize(size_t size)
{
    this->bak_size = size;
}

void MyThread::setFolder(QString folder)
{
    this->folder = folder;
}

void MyThread::setKind(int kind)
{
    this->kind = kind;
}

commandThread* MyThread::getComthread()
{
    return this->comThread;
}

void MyThread::run()
{
    if (this->kind == 0) {
        this->splitDatabaseByTime(this->dbPath);
    }
    else {
        emit SignalUpdateText("进入分叉");
        this->splitDatabaseBySize(this->dbPath);
    }
}
