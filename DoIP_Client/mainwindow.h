#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>
#include <QStandardItemModel>
#include <QString>
#include "../comm/doip.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    typedef struct
    {
        QHostAddress addr;
        quint16 port;
        struct doipPacket::VehicleAnnouncement vic;
    }VehicleInfo;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void readPendingDatagrams();

    // tcpData socket
    void TcpDataDisconnect();
    void TcpDataReadPendingDatagrams();
    void on_pushButton_start_clicked();

    void iface_refresh();

private:
    Ui::MainWindow *ui;
    QUdpSocket *Discover_Socket = NULL;
    QTcpSocket *TcpData_Client = NULL;
    QHostAddress TcpData_Server_addr;
    quint16 TcpData_Server_port;
    bool runing = false;

    QStandardItemModel *ECU_List_model;
    QList<VehicleInfo> vicInfoList;
};
#endif // MAINWINDOW_H
