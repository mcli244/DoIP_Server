#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkInterface>
#include <QDebug>
#include <QtEndian>

#include "doip.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label_state->setText("DoIP Server is stop");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    qint64 ret = -1;

    doipPacket doipPkg;
    doipPkg.VehicleAnnouncement(ui->lineEdit_VIN->text(), ui->lineEdit_logicAddr->text().toShort(NULL, 16),
                                QByteArray::fromHex(ui->lineEdit_EID->text().toUtf8()),
                                QByteArray::fromHex(ui->lineEdit_GID->text().toUtf8()), 0, 0);
    qDebug() << "DOIP Packet:" << doipPkg.Data().toHex(' ');

    ret = Discover_Socket->writeDatagram(doipPkg.Data(), QHostAddress::Broadcast, UDP_DISCOVERY);
    if(ret < 0)
        qDebug() << "writeDatagram failed";
    else
        qDebug() << "writeDatagram succ";

    ui->textBrowser->append("VehicleAnnouncement send succ");
    ui->label_state->setText("VehicleAnnouncement send succ");
}

void MainWindow::TcpDataDisconnect()
{
    TcpData_Client = 0;
    TcpData_Client_addr.clear();
    TcpData_Client_port = 0;
}


void MainWindow::TcpDataReadPendingDatagrams()
{
    //接收数据
    QByteArray arr = TcpData_Client->readAll();
    qDebug()<<"IP:" <<TcpData_Client_addr.toString() << " Port:" << TcpData_Client_port <<"recv:" << arr.toHex(' ');

    doipPacket doipMsg;
    quint16 testerAddr;

    if(doipMsg.isRoutingActivationRequst(arr))
    {
        if(doipMsg.getSourceAddrFromRoutingActivationRequst(arr, testerAddr))
        {
            doipMsg.RoutingActivationResponse(testerAddr, ui->lineEdit_logicAddr->text().toShort(NULL, 16), 0x10);
            TcpData_Client->write(doipMsg.Data());
            qDebug() << "testrAddr:" << QString::number(testerAddr, 16) << "RoutingActivationResponse:" << doipMsg.Data().toHex(' ');
            ui->textBrowser->append("testrAddr:0x" + QString::number(testerAddr, 16) + " RoutingActivationResponse:" + doipMsg.Data().toHex(' '));
            ui->label_state->setText("RoutingActivationResponse suncc");
        }
        else
        {
            qDebug() << "getSourceAddrFromRoutingActivationRequst failed";
            ui->textBrowser->append("getSourceAddrFromRoutingActivationRequst failed");
            ui->label_state->setText("RoutingActivationResponse failed");
        }
    }
}

void MainWindow::TcpDataNewConnect()
{
//    if(TcpData_Client->state() == QTcpSocket::SocketState::ConnectedState)
//    {
//        qDebug() << "已有客户端连接，拒绝本次连接";
//        return;
//    }
    TcpData_Client = TcpData_Server->nextPendingConnection();
    //打印客户端的地址
    TcpData_Client_addr = TcpData_Client->peerAddress();
    TcpData_Client_port = TcpData_Client->peerPort();

    connect(TcpData_Client, &QTcpSocket::disconnected, this, &MainWindow::TcpDataDisconnect);
    connect(TcpData_Client, &QTcpSocket::readyRead, this, &MainWindow::TcpDataReadPendingDatagrams);

    qDebug()<<"IP:" <<TcpData_Client_addr.toString() << " Port:" << TcpData_Client_port <<"连接上来!";

    ui->textBrowser->append("IP:"+TcpData_Client_addr.toString() + " Port:" + TcpData_Client_port + "connected!");

    ui->label_state->setText(QString("new client IP:"+TcpData_Client_addr.toString() + " Port:" + TcpData_Client_port));
}


void MainWindow::readPendingDatagrams()
{
    QHostAddress addr; //用于获取发送者的 IP 和端口
    quint16 port;
    //数据缓冲区
    QByteArray arr;
    while(Discover_Socket->hasPendingDatagrams())
    {
        //调整缓冲区的大小和收到的数据大小一致
        arr.resize(Discover_Socket->bytesAvailable());

        //接收数据
        Discover_Socket->readDatagram(arr.data(),arr.size(),&addr,&port);
        //显示
        qDebug() << "IP:" << addr.toString() << " Prot:" << port << "DOIP Packet:" << arr.toHex(' ');
        ui->textBrowser->append("IP:"+addr.toString() + " Port:" + port + "recv:" + arr.toHex(' '));
    }

}

void MainWindow::on_pushButton_start_clicked()
{
    if(runing)
    {
        Discover_Socket->close();
        if(TcpData_Client)
            TcpData_Client->close();
        TcpData_Server->close();
        runing = false;

        ui->label_state->setText("DoIP Server is stop");
        ui->pushButton_start->setText("启动");
    }
    else
    {
        Discover_Socket = new QUdpSocket();
        Discover_Socket->bind(QHostAddress("192.168.31.115"), UDP_DISCOVERY);   // 绑定网卡所在的IP

        TcpData_Server = new QTcpServer();
        TcpData_Server->listen(QHostAddress("192.168.31.115"), TCP_DATA);

        connect(Discover_Socket, &QUdpSocket::readyRead, this, &MainWindow::readPendingDatagrams);
        connect(TcpData_Server, &QTcpServer::newConnection, this, &MainWindow::TcpDataNewConnect);
        runing = true;

        ui->label_state->setText("DoIP Server is running");
        ui->pushButton_start->setText("停止");
    }

}

