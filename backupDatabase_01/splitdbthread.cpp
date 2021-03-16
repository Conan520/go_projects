#include "splitdbthread.h"

splitDbThread::splitDbThread()
{
    comThread = new commandThread ();
    log = new Logger();
}

splitDbThread::~splitDbThread()
{
    if (comThread)
        delete comThread;
    if (log)
        delete log;
}

void splitDbThread::setdbPath(std::string db_path)
{
    this->dbPath = db_path;
}

void splitDbThread::setfolder(QString folder)
{
    this->folder = folder;
}

void splitDbThread::set_baksize(size_t size)
{
    this->bak_size = size;
}

void splitDbThread::splitDataBase(const char *dbPath)
{
    try {
        qint64 size = QFile(dbPath).size();
        size_t file_nums = size/(bak_size*1024*1024) + 1;
        if (file_nums <= 1){
            emit updateMessage(QString::fromStdString("数据库文件过小，不宜分割"));
            log->appendLog("数据库文件过小，不宜分割");
            return ;
        }
        QFile destFile(folder + "/" + "gbak_out.txt");
        if (!destFile.open(QIODevice::WriteOnly)) {
            emit updateMessage("the file gbak_out.txt is not opened!");
            log->appendLog("the file gbak_out.txt is not opened!");
        }
        std::string command = "gbak.exe -b -user sysdba -password masterkey ";
        command += dbPath;

        QString QdbPath = dbPath;   //转为QString，方便取文件名
        QString fileName = QdbPath.right(QdbPath.size() - QdbPath.lastIndexOf('/') - 1);    //只取文件名
        fileName = folder + "/" + fileName;     //合并成新的绝对路径

        //std::string tmp_name = dbPath;
        std::string tmp_name = fileName.toStdString();
        std::string::size_type pos = tmp_name.find(".");
        char fbk_str[20] = {0};
        sprintf(fbk_str, "fbk %dm", bak_size);
        tmp_name.replace(pos + 1, pos + 4, fbk_str);
        tmp_name.insert(pos, "_%02d");
        char c = ' ';
        tmp_name.insert(tmp_name.begin(), c);
        //std::cout << tmp_name << std::endl;
        emit updateMessage("各个文件存储的路径: ");
        for (size_t i = 1; i <= file_nums; i++) {
            char temp[512] = {0};
            sprintf(temp, tmp_name.c_str(), i);
            command += temp;
            std::string file_name;      //用于将文件名保存到文件
            size_t index = tmp_name.rfind(' ');
            file_name.assign(tmp_name.begin() + 1, tmp_name.begin() + index);		//文件前面有一个空格
            memset(temp, 0, sizeof(temp));
            sprintf(temp, file_name.c_str(), i);
            destFile.write(temp);
            destFile.write("\n");
            destFile.flush();

            emit updateMessage(QString::fromStdString(temp));
            log->appendLog(QString::fromStdString(temp));
            //this->repaint();
        }
        destFile.close();
        this->comThread->setCommand(command);
        this->comThread->start();
        while(!this->comThread->isFinished());
        //表示数据库文件已经备份好了，可以开始传送了
        QFile stateFile(folder + "/" + "gbak_state.txt");
        if(stateFile.open(QIODevice::WriteOnly)){
            stateFile.write("1");
        }
        stateFile.close();
    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }
}

commandThread *splitDbThread::getThread()
{
    return this->comThread;
}

void splitDbThread::run()
{
    splitDataBase(dbPath.c_str());
}
