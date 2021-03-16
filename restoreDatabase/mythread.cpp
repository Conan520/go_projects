#include "mythread.h"

myThread::myThread()
{
    log = new Logger();
}

myThread::~myThread()
{
    if(log)
        delete log;
}

void myThread::run()
{
    exeuteCommand(this->command.c_str());
}

void myThread::setCommand(std::string command)
{
    this->command = command;
}

void myThread::exeuteCommand(const char *command)
{
    try {
        SignalUpdateText(QString::fromStdString("合并恢复中，请稍等······"));
        log->appendLog("合并恢复中，请稍等······");
        int res = QProcess::execute(command);      //执行命令进程并等待进程结束
        if ( res == 0 ) {
            SignalUpdateText(QString::fromStdString("合并恢复成功"));
            log->appendLog("合并恢复成功");
        }
        else {
            SignalUpdateText(QString::fromStdString("合并恢复失败，请重新检查所选的备份文件"));
            log->appendLog("合并恢复失败，请重新检查所选的备份文件");
        }
    } catch (QException* e) {
        log->appendLog(e->what());
    } catch (std::exception *e) {
        log->appendLog(e->what());
    }

}
