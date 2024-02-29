#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkInterface>
#include <QDebug>
#include <QtEndian>
#include <QTimer>
#include <QStandardItemModel>
#include <QThread>
#include <cstring>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label_state->setText("DoIP Client is stop");

    // 禁用 车辆声明广播按钮
    ui->pushButton->setEnabled(false);

    // ECU DID
    // 创建一个标准的表格模型
    ECU_List_model = new QStandardItemModel(0,5);
    ECU_List_model->setHorizontalHeaderLabels(QStringList() << "IP地址:端口" << "逻辑地址" << "VIN" << "EID" << "GID");

    ui->tableView_ECU_List->setModel(ECU_List_model);
    ui->tableView_ECU_List->resizeColumnsToContents();
    ui->tableView_ECU_List->show();

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
        //qDebug() << "Interface Name:" << interface.name();
        //qDebug() << "Hardware Address:" << interface.hardwareAddress();

        // 打印IPv4地址
        QList<QNetworkAddressEntry> entries = interface.addressEntries();
        foreach (QNetworkAddressEntry entry, entries) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                //qDebug() << "IPv4 Address:" << entry.ip().toString();
                //qDebug() << "Netmask:" << entry.netmask().toString();
                ui->comboBox_iface->addItem(entry.ip().toString());
            }
        }

        //qDebug() << "-----------------------------------";
    }
}

void MainWindow::on_pushButton_clicked()
{
    doipPacket doipMsg;

    doipMsg.VehicleIdentificationRequest();
    if(Discover_Socket->writeDatagram(doipMsg.Data(), QHostAddress::Broadcast, 13400))
        qDebug() << "writeDatagram succ";
    else
        qDebug() << "writeDatagram failed";

    ui->textBrowser->append("VehicleIdentificationRequest send succ");
    ui->label_state->setText("VehicleIdentificationRequest send succ");
}

void MainWindow::TcpDataDisconnect()
{
//    TcpData_Client = 0;
//    TcpData_Client_addr.clear();
//    TcpData_Client_port = 0;
}


void MainWindow::TcpDataReadPendingDatagrams()
{
//    //接收数据
//    QByteArray arr = TcpData_Client->readAll();
//    qDebug()<<"IP:" <<TcpData_Client_addr.toString() << " Port:" << TcpData_Client_port <<"recv:" << arr.toHex(' ');

//    doipPacket doipMsg(arr);
//    quint16 testerAddr;

//    if(doipMsg.isRoutingActivationRequst())
//    {
//        if(doipMsg.getSourceAddrFromRoutingActivationRequst(testerAddr))
//        {
//            doipPacket doipRes;
//            if(doipRes.RoutingActivationResponse(testerAddr, ui->lineEdit_logicAddr->text().toShort(NULL, 16), 0x10))
//            {
//                TcpData_Client->write(doipRes.Data());

//                qDebug() << "testrAddr:" << QString::number(testerAddr, 16) << "RoutingActivationResponse:" << doipRes.Data().toHex(' ');
//                ui->textBrowser->append("testrAddr:0x" + QString::number(testerAddr, 16) + " RoutingActivationResponse:" + doipRes.Data().toHex(' '));
//                ui->label_state->setText("RoutingActivationResponse suncc");
//            }
//            else
//                qDebug() << "creat RoutingActivationResponse pkg filed";
//        }
//        else
//        {
//            qDebug() << "getSourceAddrFromRoutingActivationRequst failed";
//            ui->textBrowser->append("getSourceAddrFromRoutingActivationRequst failed");
//            ui->label_state->setText("RoutingActivationResponse failed");
//        }
//    } else if(doipMsg.isDiagnosticMsg())
//    {
//        struct doipPacket::DiagnosticMsg dmsg;
//        if(doipMsg.DiagnosticMsgAnalyze(dmsg))
//        {
//            QByteArray resp;
//            QString result = QString("DOIP DiagnosticMsgAnalyze type: " + QString::number(dmsg.type)
//                             + " sourceAddr: " + QString::number(dmsg.sourceAddr, 16)
//                             + " targetAddr: " + QString::number(dmsg.targetAddr, 16)
//                             + " uds:" + dmsg.udsData.toHex(' '));
//            qDebug() << result;
//            ui->textBrowser->append(result);

//            doipPacket doipResACK;

//            // 回应DOIP ACK
//            if(doipResACK.DiagnosticMsgACK(ui->lineEdit_logicAddr->text().toShort(NULL, 16), dmsg.sourceAddr, 0x00))
//            {
//                qDebug() << "DiagnosticMsgACK:" << doipResACK.Data().toHex(' ');
//                TcpData_Client->write(doipResACK.Data());
//                TcpData_Client->flush();    //将缓冲区的数据全部发送出去，不然这个的ACK和下面的MSG包会黏在一起发出去
//            }

//            // 回应 MSG
//            doipPacket doipRes;
//            DiagnosticMsgPro(dmsg, resp);
//            if(doipRes.DiagnosticMsg(ui->lineEdit_logicAddr->text().toShort(NULL, 16), dmsg.sourceAddr,  resp))
//            {
//                TcpData_Client->write(doipRes.Data());

//                qDebug() << "testrAddr:" << ui->lineEdit_logicAddr->text() << "DiagnosticMsg:" << doipRes.Data().toHex(' ');
//                ui->textBrowser->append("testrAddr:0x" + ui->lineEdit_logicAddr->text() + " DiagnosticMsg:" + doipRes.Data().toHex(' '));
//                ui->label_state->setText("RoutingActivationResponse suncc");
//            }
//            else
//                qDebug() << "creat RoutingActivationResponse pkg filed";
//        }
//        else
//            qDebug() << "DiagnosticMsgAnalyze failed";

//    }
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
        arr.clear();
        arr.resize(Discover_Socket->bytesAvailable());

        //接收数据
        Discover_Socket->readDatagram(arr.data(),arr.size(),&addr,&port);

        doipPacket recv(arr);

        struct doipPacket::VehicleAnnouncement vic;
        if(recv.VehicleAnnouncementAnalyze(vic))
        {
            TcpData_Server_addr = addr;
            TcpData_Server_port = port;

            //显示
            qDebug() << "IP:" << addr.toString() << " Prot:" << QString::number(port) << "DOIP Packet:" << arr.toHex(' ');
            qDebug() << "VIN:" << vic.VIN
                     << "logicalAddr" << QString::asprintf("0x%4X", vic.logicalAddr)
                     << "EID:" << vic.EID.toHex(' ')
                     << "GID:" << vic.GID.toHex(' ')
                     << "Fur:" << QString::asprintf("0x%2X", vic.Fur)
                     << "syncSta:" << QString::asprintf("0x%2X", vic.syncSta);
            ui->textBrowser->append("IP:"+addr.toString() + " Port:" + QString::number(port) + "recv:" + arr.toHex(' '));

            // TODO: 检查现存的信息中是否已经记录了，没记录就插入
            VehicleInfo vicInfo;
            vicInfo.addr = addr;
            vicInfo.port = port;
            vicInfo.vic = vic;

            if(0 == vicInfoList.size())
            {
                qDebug() << "新的车辆信息已记录" << "size:" << QString::number(vicInfoList.size());
                QList<QStandardItem *> aitemlist;
                aitemlist.append(new QStandardItem(addr.toString()+":"+QString::number(port)));
                aitemlist.append(new QStandardItem(QString::asprintf("0x%04X", vic.logicalAddr)));
                aitemlist.append(new QStandardItem(vic.VIN));
                aitemlist.append(new QStandardItem(QString(vic.EID.toHex())));
                aitemlist.append(new QStandardItem(QString(vic.GID.toHex())));
                ECU_List_model->insertRow(ECU_List_model->rowCount(),aitemlist);
                vicInfoList.append(vicInfo);
            }
            else
            {
                bool nonexistent = true;
                for(int i=0; i<vicInfoList.size(); i++)
                {
                    VehicleInfo *node = (VehicleInfo *)&vicInfoList.at(i);
                    if(node->addr == vicInfo.addr
                        && node->port == vicInfo.port
                        && node->vic.logicalAddr == vicInfo.vic.logicalAddr
                        && node->vic.VIN == vicInfo.vic.VIN
                        && node->vic.EID == vicInfo.vic.EID
                        && node->vic.GID == vicInfo.vic.GID)
//                    if(memcmp(node, &vicInfo, sizeof(VehicleInfo)) == 0)
                    {
                        qDebug() << "车辆信息已经存在！";
                        nonexistent = false;
                        break;
                    }
                }

                if(nonexistent)
                {
                    qDebug() << "新的车辆信息已记录 " << "size:" << QString::number(vicInfoList.size());
                    QList<QStandardItem *> aitemlist;
                    aitemlist.append(new QStandardItem(addr.toString()+":"+QString::number(port)));
                    aitemlist.append(new QStandardItem(QString::asprintf("0x%04X", vic.logicalAddr)));
                    aitemlist.append(new QStandardItem(vic.VIN));
                    aitemlist.append(new QStandardItem(QString(vic.EID.toHex())));
                    aitemlist.append(new QStandardItem(QString(vic.GID.toHex())));
                    ECU_List_model->insertRow(ECU_List_model->rowCount(),aitemlist);
                    vicInfoList.append(vicInfo);
                }

            }

        }


    }

}

void MainWindow::on_pushButton_start_clicked()
{
    if(runing)
    {
        Discover_Socket->close();
        if(TcpData_Client)
            TcpData_Client->close();
        runing = false;

        // 启用ECU信息编辑
        ui->lineEdit_logicAddr->setEnabled(true);
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

        connect(Discover_Socket, &QUdpSocket::readyRead, this, &MainWindow::readPendingDatagrams);

        runing = true;

        // 禁用ECU信息编辑
        ui->lineEdit_logicAddr->setEnabled(false);
        ui->lineEdit_UDP_DISCOVER->setEnabled(false);

        ui->pushButton_refresh->setEnabled(false);

        // 启用 车辆声明广播按钮
        ui->pushButton->setEnabled(true);

        ui->textBrowser->append("Discover       (UDP): " + ui->comboBox_iface->currentText() + " Port: " + ui->lineEdit_UDP_DISCOVER->text());
        ui->label_state->setText("DoIP Client is running");
        ui->pushButton_start->setText("停止");
    }

}


