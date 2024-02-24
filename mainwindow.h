#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

const int TCP_DATA = 13400;
const int UDP_DISCOVERY = 13400;

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

private:
    Ui::MainWindow *ui;
    QUdpSocket *Discover_Socket;
    QTcpServer *TcpData_Server;
    QTcpSocket *TcpData_Client;
    QHostAddress TcpData_Client_addr;
    quint16 TcpData_Client_port;
    bool runing = false;
};
#endif // MAINWINDOW_H
