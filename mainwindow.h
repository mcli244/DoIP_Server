#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>

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

private:
    Ui::MainWindow *ui;
    QUdpSocket *Discover_Socket = NULL;
    QTcpServer *TcpData_Server = NULL;
    QTcpSocket *TcpData_Client = NULL;
    QHostAddress TcpData_Client_addr;
    quint16 TcpData_Client_port;
    bool runing = false;

    QTimer *Discover_timer = NULL;
};
#endif // MAINWINDOW_H
