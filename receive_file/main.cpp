#include "receivefile.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ReceiveFile w;
    w.show();
    return a.exec();
}
