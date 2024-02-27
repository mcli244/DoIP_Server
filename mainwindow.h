#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>
#include <QStandardItemModel>

#include "doip.h"

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
    void on_pushButton_clicked();

    void readPendingDatagrams();

    // tcpData socket
    void TcpDataNewConnect();
    void TcpDataDisconnect();
    void TcpDataReadPendingDatagrams();
    void on_pushButton_start_clicked();

    void iface_refresh();
    bool sendVehicleAnnouncement(QHostAddress addr, quint16 port);

    void on_pushButton_add_col_clicked();

    void on_pushButton_add_row_clicked();

    void on_pushButton_del_col_clicked();

    void on_pushButton_del_row_clicked();

    void on_pushButton_ECU_DID_Refresh_clicked();

private:
    Ui::MainWindow *ui;
    QUdpSocket *Discover_Socket = NULL;
    QTcpServer *TcpData_Server = NULL;
    QTcpSocket *TcpData_Client = NULL;
    QHostAddress TcpData_Client_addr;
    quint16 TcpData_Client_port;
    bool runing = false;

    QTimer *Discover_timer = NULL;
    QStandardItemModel *ECU_DID_model;

    bool DiagnosticMsgPro(QByteArray &uds, QByteArray &resp);
    bool getValueByDID(quint32 did, QByteArray &value);
    void DiagnosticMsgPro(struct doipPacket::DiagnosticMsg &dmsg, QByteArray &resp);
};
#endif // MAINWINDOW_H
