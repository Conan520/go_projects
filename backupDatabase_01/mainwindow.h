#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mythread.h"
#include "commandthread.h"
#include "splitdbthread.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_toolButton_clicked();
    void slotUpdateText(QString str);

    void setBackupMode();

    QString getTime();

    void on_pBtnStart_clicked();

    void on_pBtnExit_clicked();

    void on_selectFolderBtn_clicked();

private:
    Ui::MainWindow *ui;
    MyThread *mthread;
    commandThread *comThread;
    splitDbThread *spThread;
    QString folder;
};
#endif // MAINWINDOW_H
