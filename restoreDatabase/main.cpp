#include "rmainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    rMainWindow w;
    //w.resize(1000, 1000);
    w.setMaximumSize(940, 715);
    w.setMinimumSize(940, 715);
    w.show();
    return a.exec();
}
