#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QLabel> // 包含 QLabel 头文件

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
    void readImage(); // 接收图像数据的槽函数

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;
    QLabel *imageLabel; // QLabel 控件用于显示图像
};

#endif // MAINWINDOW_H
