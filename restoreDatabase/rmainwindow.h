#ifndef RMAINWINDOW_H
#define RMAINWINDOW_H
#include <QMainWindow>
#include "mythread.h"
#include "mergeThread.h"
#include "mergetimethread.h"
#include <set>
#include "logger.h"

QT_BEGIN_NAMESPACE
namespace Ui { class rMainWindow; }
QT_END_NAMESPACE

class rMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    rMainWindow(QWidget *parent = nullptr);
    ~rMainWindow();

private slots:
    void on_toolButton_clicked();

    void on_pBtnStart_clicked();

    QString getTime();

    void mergeDBbysize();

    void mergeDBbyTime();

    std::set<std::string> read_file(std::string);

    void slotUpdateText(QString str);

    void on_selectDirBtn_clicked();

    void on_pBtnExit_clicked();

private:
    Ui::rMainWindow *ui;
    myThread *mthread;
    mergeThread* mr;
    MergeTimeThread* mt;
    QString directory;
    Logger *log;
};
#endif // RMAINWINDOW_H
