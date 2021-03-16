#include "mainwindow.h"

#include <QApplication>
#include <QDate>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    //限制使用时间
    QDateTime time = QDateTime::currentDateTime(); //获取当前时间
    size_t cur_timestamp = time.toTime_t(); //将当前时间转为时间戳

    QDateTime date = QDateTime ::fromString("20210407120000", "yyyyMMddhhmmss");
    size_t used_time = date.toTime_t();
    if (cur_timestamp > used_time) {
        w.close();
        return 0;
    }
    else {
        w.show();
        return a.exec();
    }
}
