#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>
#include <QStandardItemModel>
#include <QString>
#include <QNetworkProxy>
#include "../comm/doip.h"
#include "../comm/uds.h"

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

    void on_pushButton_vic_connect_clicked();

    void on_pushButton_readDID_clicked();

    void on_pushButton_readDTC_clicked();

    void on_pushButton_readAllDTC_clicked();

    void on_pushButton_cleanDTC_clicked();

private:
    Ui::MainWindow *ui;
    QUdpSocket *Discover_Socket = NULL;
    QTcpSocket *TcpData_Client = NULL;
    VehicleInfo *curVIC = NULL;
    bool runing = false;

    QStandardItemModel *ECU_List_model;
    QStandardItemModel *ECU_List_model_dtc;
    QList<VehicleInfo> vicInfoList;
    bool vicIsConnect = false;
    bool BytesToDTC(QByteArray &dtc, QString &str);
    bool udsPro(QByteArray &uds);
    void sendUdsComm(QByteArray &uds);
};
#endif // MAINWINDOW_H
