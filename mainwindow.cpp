#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkInterface>
#include <QDebug>
#include <QtEndian>
#include <QTimer>

#include "doip.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label_state->setText("DoIP Server is stop");

    // 禁用 车辆声明广播按钮
    ui->pushButton->setEnabled(false);

    iface_refresh();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::iface_refresh()
{
    // 获取系统中的所有网络接口
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    // 遍历所有网络接口
    foreach (QNetworkInterface interface, interfaces) {
        // 打印接口名称和硬件地址
        qDebug() << "Interface Name:" << interface.name();
        qDebug() << "Hardware Address:" << interface.hardwareAddress();

        // 打印IPv4地址
        QList<QNetworkAddressEntry> entries = interface.addressEntries();
        foreach (QNetworkAddressEntry entry, entries) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                qDebug() << "IPv4 Address:" << entry.ip().toString();
                qDebug() << "Netmask:" << entry.netmask().toString();
                ui->comboBox_iface->addItem(entry.ip().toString());
            }
        }

        qDebug() << "-----------------------------------";
    }
}

bool MainWindow::sendVehicleAnnouncement(QHostAddress addr, quint16 port)
{
    doipPacket doipPkg;
    doipPkg.VehicleAnnouncement(ui->lineEdit_VIN->text(), ui->lineEdit_logicAddr->text().toShort(NULL, 16),
                                QByteArray::fromHex(ui->lineEdit_EID->text().toUtf8()),
                                QByteArray::fromHex(ui->lineEdit_GID->text().toUtf8()), 0, 0);
    qDebug() << "DOIP Packet:" << doipPkg.Data().toHex(' ');

    return Discover_Socket->writeDatagram(doipPkg.Data(), addr, port);
}

void MainWindow::on_pushButton_clicked()
{
    if(sendVehicleAnnouncement(QHostAddress::Broadcast, ui->lineEdit_UDP_DISCOVER->text().toInt(NULL)))
        qDebug() << "writeDatagram succ";
    else
        qDebug() << "writeDatagram failed";

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
        qDebug() << "IP:" << addr.toString() << " Prot:" << QString::number(port) << "DOIP Packet:" << arr.toHex(' ');
        ui->textBrowser->append("IP:"+addr.toString() + " Port:" + QString::number(port) + "recv:" + arr.toHex(' '));
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

        // 启用ECU信息编辑
        ui->lineEdit_logicAddr->setEnabled(true);
        ui->lineEdit_VIN->setEnabled(true);
        ui->lineEdit_EID->setEnabled(true);
        ui->lineEdit_GID->setEnabled(true);

        ui->lineEdit_TCP_DATA->setEnabled(true);
        ui->lineEdit_UDP_DISCOVER->setEnabled(true);

        ui->pushButton_refresh->setEnabled(true);

        // 禁用 车辆声明广播按钮
        ui->pushButton->setEnabled(false);

        ui->label_state->setText("DoIP Server is stop");
        ui->pushButton_start->setText("启动");
    }
    else
    {
        Discover_Socket = new QUdpSocket();

        if(false == Discover_Socket->bind(QHostAddress(ui->comboBox_iface->currentText()), ui->lineEdit_UDP_DISCOVER->text().toInt(NULL)))   // 绑定网卡所在的IP
        {
            ui->label_state->setText("Discover IP地址绑定失败，检查IP地址是否可用！");
            return;
        }

        TcpData_Server = new QTcpServer();
        if(false == TcpData_Server->listen(QHostAddress(ui->comboBox_iface->currentText()), ui->lineEdit_TCP_DATA->text().toInt(NULL)))
        {
            Discover_Socket->close();
            ui->label_state->setText("TcpData IP地址绑定失败，检查IP地址是否可用！");
            return;
        }

        connect(Discover_Socket, &QUdpSocket::readyRead, this, &MainWindow::readPendingDatagrams);
        connect(TcpData_Server, &QTcpServer::newConnection, this, &MainWindow::TcpDataNewConnect);
        runing = true;

        // 禁用ECU信息编辑
        ui->lineEdit_logicAddr->setEnabled(false);
        ui->lineEdit_VIN->setEnabled(false);
        ui->lineEdit_EID->setEnabled(false);
        ui->lineEdit_GID->setEnabled(false);

        ui->lineEdit_TCP_DATA->setEnabled(false);
        ui->lineEdit_UDP_DISCOVER->setEnabled(false);

        ui->pushButton_refresh->setEnabled(false);

        // 启用 车辆声明广播按钮
        ui->pushButton->setEnabled(true);

        ui->textBrowser->append("Discover       (UDP): " + ui->comboBox_iface->currentText() + " Port: " + ui->lineEdit_UDP_DISCOVER->text());
        ui->textBrowser->append("TCP_Data_Server(TCP): " + ui->comboBox_iface->currentText() + " Port: " + ui->lineEdit_TCP_DATA->text());
        ui->label_state->setText("DoIP Server is running");
        ui->pushButton_start->setText("停止");

        // 启动定时器，间隔150ms发送车辆声明 (DOIP协议约定，在500ms内，发送三次)
        Discover_timer = new QTimer();
        Discover_timer->setInterval(150);
        Discover_timer->start();
        connect(Discover_timer, &QTimer::timeout, [=](){
            static int cnt = 0;

            if(sendVehicleAnnouncement(QHostAddress::Broadcast, ui->lineEdit_UDP_DISCOVER->text().toInt(NULL)))
                qDebug() << "writeDatagram succ";
            else
                qDebug() << "writeDatagram failed";

            ui->textBrowser->append("AUTO VehicleAnnouncement send succ");
            ui->label_state->setText("AUTO  VehicleAnnouncement send succ");

            cnt++;
            if(cnt >= 3)
                Discover_timer->stop();
        });
    }

}

