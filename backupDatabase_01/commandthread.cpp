#include "commandthread.h"

commandThread::commandThread()
{
    log = new Logger();
}

commandThread::~commandThread()
{
    if (log)
        delete log;
}

void commandThread::setCommand(std::string command)
{
    this->command = command;
}

void commandThread::exeuteCommand(const char *command)
{
    try {
        UpdateText(QString::fromStdString("数据存储中,请稍等······"));
        log->appendLog("数据存储中,请稍等······");
        //UpdateText(QString::fromStdString(command));
        int res = QProcess::execute(command);
        if (res == 0) {
            UpdateText(QString::fromStdString("数据此次存储成功"));
            log->appendLog("数据此次存储成功");
        }
        else {
            UpdateText(QString::fromStdString("数据此次存储失败，请检查是否存在重名文件！"));
            log->appendLog("数据此次存储失败，请检查是否存在重名文件！");
        }
    } catch (QException *e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }

}

void commandThread::run()
{
    exeuteCommand(this->command.c_str());
}
