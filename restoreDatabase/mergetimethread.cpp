#include "mergetimethread.h"

MergeTimeThread::MergeTimeThread()
{
    mthread = new myThread();
    log = new Logger();
}

MergeTimeThread::~MergeTimeThread()
{
    delete mthread;
    delete log;
}

myThread *MergeTimeThread::getThread()
{
    return this->mthread;
}

std::set<std::string> MergeTimeThread::read_file(std::string m_file)
{
    try {
        QFile nbk_state(directory + "/" + "nbk_state.txt");
        while (!nbk_state.exists());    //如果文件不存在，则阻塞

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
    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }

}

void MergeTimeThread::mergeDBbyTime()
{
    try {
        std::vector<std::string> file_names;
        QString file_name;

        std::set<std::string> result = read_file("nbk_out.txt");

        for(auto i=result.begin(); i != result.end(); i++) {
            file_names.emplace_back(*i);
        }
        //std::sort(file_names.begin(), file_names.end());

        std::string command = "nbackup -R ";
        std::string temp_file = file_names[file_names.size() - 1];      //temp_file最终为dbPath数据库名绝对路径
        std::string::size_type pos_1 = temp_file.find('_');
        std::string::size_type pos_2 = temp_file.find('.');
        if ( pos_2 == std::string::npos ) {
            emit updateText(QString::fromStdString("未找到."));
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
        emit updateText("数据库文件的路径：" + QString::fromStdString(temp_file));
        log->appendLog(QString("数据库文件的路径: %1").arg(temp_file.c_str()));
        this->mthread->setCommand(command);
        this->mthread->start();
    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }

}

std::string MergeTimeThread::get_time()
{
    const time_t t = time(nullptr);
    tm* local; //本地时间
    char buf[128]= {0};
    local = localtime(&t); //转为本地时间
    strftime(buf, 64, "%Y%m%d", local);
    std::string res = buf;
    return res;
}

void MergeTimeThread::setDirectory(QString dir)
{
    this->directory = dir;
}

void MergeTimeThread::setDays(unsigned long days)
{
    this->days = days;
}

void MergeTimeThread::run()
{
    while (true) {
        mergeDBbyTime();
        sleep(days*86400);      //线程QThread::sleep()
    }
}
