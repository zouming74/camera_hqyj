#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QPixmap>
#include <QByteArray>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket(new QTcpSocket(this))
{
    ui->setupUi(this);

    imageLabel = ui->imageLabel; // 将 imageLabel 指向 ui 中的 imageLabel 控件

    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readImage);

    socket->connectToHost("192.168.98.219", 8080); // 替换为实际的虚拟机 IP 和端口
    if (socket->waitForConnected(5000)) {
        qDebug() << "连接到服务器成功";
    } else {
        qDebug() << "连接到服务器失败";
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

//void MainWindow::readImage()
//{
//    static bool receivingImage = false;
//    static qint64 imageSize = 0;
//    static QByteArray imageData;

//    while (socket->bytesAvailable() > 0)
//    {
//        if (!receivingImage)
//        {
//            QByteArray sizeData = socket->readLine();
//            imageSize = sizeData.trimmed().toLongLong();
//            imageData.clear();
//            receivingImage = true;
//            qDebug() << "准备接收图像数据，大小：" << imageSize;
//        }
//        else
//        {
//            imageData.append(socket->read(imageSize - imageData.size()));
//            qDebug() << "已接收数据大小：" << imageData.size() << "/" << imageSize;
//            if (imageData.size() >= imageSize)
//            {
//                receivingImage = false;
//                QPixmap pixmap;
//                pixmap.loadFromData(imageData);
//                if (!pixmap.isNull()) {
//                    imageLabel->setPixmap(pixmap); // 设置 QLabel 显示图像
//                    imageLabel->setScaledContents(true); // 可选，保证图像自适应大小
//                    imageLabel->update(); // 确保更新显示
//                    qDebug() << "图像接收并显示成功";
//                } else {
//                    qDebug() << "图像数据错误：无法加载图像";
//                }

//                // 清空图像数据，准备接收下一张图像
//                imageData.clear();
//                imageSize = 0;
//            }
//        }
//    }
//}
void MainWindow::readImage()
{
    static QByteArray imageData;

    // Append all available data to the image data buffer
    imageData.append(socket->readAll());

    // Look for the start of the MJPEG image
    int jpegStart = imageData.indexOf(QByteArray("\xFF\xD8"));
    if (jpegStart != -1) {
        // Look for the end of the MJPEG image
        int jpegEnd = imageData.indexOf(QByteArray("\xFF\xD9"), jpegStart);
        if (jpegEnd != -1) {
            // Extract the JPEG data between jpegStart and jpegEnd
            QByteArray jpegData = imageData.mid(jpegStart, jpegEnd - jpegStart + 2);

            // Remove processed data from the buffer
            imageData.remove(0, jpegEnd + 2);

            // Display the MJPEG image
            QPixmap pixmap;
            pixmap.loadFromData(jpegData);
            if (!pixmap.isNull()) {
                imageLabel->setPixmap(pixmap);
                imageLabel->setScaledContents(true);
                imageLabel->update();
                qDebug() << "Received and displayed MJPEG image successfully";
            } else {
                qDebug() << "Failed to load MJPEG image data";
            }
        }
    }
}
