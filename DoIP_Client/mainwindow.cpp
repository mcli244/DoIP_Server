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
    ECU_List_model = new QStandardItemModel(0,6);
    ECU_List_model->setHorizontalHeaderLabels(QStringList() << "VIN" << "逻辑地址"  << "EID" << "GID" << "IP" << "端口");

    ui->tableView_ECU_List->setModel(ECU_List_model);
    ui->tableView_ECU_List->resizeColumnsToContents();
    ui->tableView_ECU_List->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView_ECU_List->show();

    TcpData_Client = new QTcpSocket(this);
    TcpData_Client->setProxy(QNetworkProxy::NoProxy);   // 去掉QT的系统代理，不然连不上 报错The proxy type is invalid for this operation
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


bool MainWindow::BytesToDTC(QByteArray &dtc, QString &str)
{
    if(dtc.size() != 4)  return false;

    str.resize(5);

    switch(dtc.at(0)>>6 & 0x03)
    {
        case 0: str = 'P';   break;
        case 1: str = 'C';   break;
        case 2: str = 'B';   break;
        case 3: str = 'U';   break;
        default: return false;
    }

    switch(dtc.at(0)>>4 & 0x03)
    {
        case 0: str += '0';   break;
        case 1: str += '1';   break;
        case 2: str += '2';   break;
        case 3: str += '3';   break;
        default: return false;
    }

    switch(dtc.at(0) & 0x0F)
    {
    case 0: str += '0';   break;
    case 1: str += '1';   break;
    case 2: str += '2';   break;
    case 3: str += '3';   break;
    case 4: str += '4';   break;
    case 5: str += '5';   break;
    case 6: str += '6';   break;
    case 7: str += '7';   break;
    case 8: str += '8';   break;
    default: return false;
    }

    switch(dtc.at(1)>>4 & 0x0F)
    {
    case 0: str += '0';   break;
    case 1: str += '1';   break;
    case 2: str += '2';   break;
    case 3: str += '3';   break;
    case 4: str += '4';   break;
    case 5: str += '5';   break;
    case 6: str += '6';   break;
    case 7: str += '7';   break;
    case 8: str += '8';   break;
    default: return false;
    }

    switch(dtc.at(1) & 0x0F)
    {
    case 0: str += '0';   break;
    case 1: str += '1';   break;
    case 2: str += '2';   break;
    case 3: str += '3';   break;
    case 4: str += '4';   break;
    case 5: str += '5';   break;
    case 6: str += '6';   break;
    case 7: str += '7';   break;
    case 8: str += '8';   break;
    default: return false;
    }

    qDebug() << "DTC: " << dtc.toHex(' ') << "错误码:" << str;

    return true;
}

bool MainWindow::udsPrint(QByteArray &uds, QString &inf)
{
    if(!uds.size()) return false;

    if(uds.at(0) == 0x7f)
    {
        inf = "否等响应\n";
        inf += "服务: " + QString::number(uds.at(1), 16) + "\n";
        inf += "数据: " + uds.mid(3, uds.size()).toHex(' ') + "\n";
        return true;
    }

    inf = "肯定响应\n";
    inf += "服务: " + QString::number(uds.at(0)-0x40, 16) + "\n";
    switch(uds.at(0) - 0x40)
    {
    case 0x22:
        inf += "DID: " + uds.mid(3, 2).toHex(' ') + "\n";
        inf += "数据: " + uds.mid(5, uds.size()).toHex(' ') + "\n";
        break;
    case 0x19:
        switch(uds.at(1))
        {
        case 0x02:
            inf += "mask: " + QString::number(uds.at(2), 16) + "\n";
            for(int i=3; i<uds.size(); i+=4)
            {
                QString code;
                QByteArray dtc = uds.mid(i, 4);
                if(BytesToDTC(dtc, code))
                {
                    inf += "DTC: " + code + "\n";
                }
                else
                    inf += "DTC: null \n";

            }

            break;
        case 0x01:
        case 0x03:
        case 0x04:
        case 0x0A:
        default:
            break;
        }
    default:
        break;
    }

    return true;
}

void MainWindow::TcpDataReadPendingDatagrams()
{
    //接收数据
    QByteArray arr = TcpData_Client->readAll();
    qDebug()<<"recv:" << arr.toHex(' ');

    doipPacket doipMsg(arr);
    quint16 targetAddr;
    quint16 sourceAddr;
    quint8 rescode;
    struct doipPacket::DiagnosticMsg diagInfo;

    if(doipMsg.RoutingActivationResponseAnalyze(targetAddr, sourceAddr, rescode))
    {
        if(sourceAddr == curVIC->vic.logicalAddr
            && targetAddr == ui->lineEdit_logicAddr->text().toUShort(NULL, 16))
        {
            // 确认是来自请求车辆的DOIP路由激活响应
            if(rescode == 0x10)
            {
                ui->textBrowser->append("车辆连接成功");
                ui->pushButton->setText("车辆断开");
                vicIsConnect = true;
            }

        }
        else
        {
            qDebug() << "信息不匹配"
                     << "sourceAddr:" << QString::number(sourceAddr, 16)
                     << "curVIC->vic.logicalAddr:"<<QString::number(curVIC->vic.logicalAddr, 16)
                     << "targetAddr" << QString::number(targetAddr, 16)
                     << "ui->lineEdit_logicAddr->text().toUShort(NULL, 16)" << QString::number(ui->lineEdit_logicAddr->text().toUShort(NULL, 16), 16);
        }
    }
    else if(doipMsg.DiagnosticMsgAnalyze(diagInfo))
    {
        if(diagInfo.type == doipPacket::MSG)
        {
            qDebug() << "诊断报文:" << diagInfo.udsData.toHex(' ');
            ui->textBrowser->append("诊断报文:" + diagInfo.udsData.toHex(' '));
            QString udsInfo;
            QByteArray uds;
            uds.append(diagInfo.udsData);
            udsPrint(uds, udsInfo);
            qDebug() << udsInfo;
        }
    }
    else
    {
        qDebug() << "不是DOIP报文";
    }
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
                QList<QStandardItem *> aitemlist;

                // 注意顺序 "VIN" << "逻辑地址"  << "EID" << "GID" << "IP" << "端口"
                aitemlist.append(new QStandardItem(vic.VIN));
                aitemlist.append(new QStandardItem(QString::asprintf("0x%04X", vic.logicalAddr)));
                aitemlist.append(new QStandardItem(QString(vic.EID.toHex())));
                aitemlist.append(new QStandardItem(QString(vic.GID.toHex())));
                aitemlist.append(new QStandardItem(addr.toString()));
                aitemlist.append(new QStandardItem(QString::number(port)));

                ECU_List_model->insertRow(ECU_List_model->rowCount(),aitemlist);
                vicInfoList.append(vicInfo);
                qDebug() << "新的车辆信息已记录" << "size:" << QString::number(vicInfoList.size());
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
                    QList<QStandardItem *> aitemlist;
                    aitemlist.append(new QStandardItem(addr.toString()+":"+QString::number(port)));
                    aitemlist.append(new QStandardItem(QString::asprintf("0x%04X", vic.logicalAddr)));
                    aitemlist.append(new QStandardItem(vic.VIN));
                    aitemlist.append(new QStandardItem(QString(vic.EID.toHex())));
                    aitemlist.append(new QStandardItem(QString(vic.GID.toHex())));
                    ECU_List_model->insertRow(ECU_List_model->rowCount(),aitemlist);

                    qDebug() << "新的车辆信息已记录 " << "size:" << QString::number(vicInfoList.size());
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

        bool ret = false;
        if(ui->lineEdit_UDP_DISCOVER->text().toInt(NULL))
            ret = Discover_Socket->bind(QHostAddress(ui->comboBox_iface->currentText()), ui->lineEdit_UDP_DISCOVER->text().toInt(NULL));
        else
            ret = Discover_Socket->bind(QHostAddress(ui->comboBox_iface->currentText()));
        if(false == ret)   // 绑定网卡所在的IP
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

void MainWindow::on_pushButton_vic_connect_clicked()
{
    // 根据选中的车辆，发送路由激活请求(DOIP)
    if(vicIsConnect)
    {
        TcpData_Client->disconnect();
        ui->textBrowser->append("车辆已断开");
        ui->pushButton->setText("车辆连接");
        ui->pushButton_vic_connect->setText("连接车辆");
        ui->pushButton->setEnabled(true);
        vicIsConnect = false;
    }
    else
    {
        // 注意顺序 "VIN" << "逻辑地址"  << "EID" << "GID" << "IP" << "端口"
        QModelIndex curIndex = ui->tableView_ECU_List->currentIndex();
        QString vic_IP = ECU_List_model->data(ECU_List_model->index(curIndex.row(), 4)).toString();
        quint16 port = ECU_List_model->data(ECU_List_model->index(curIndex.row(), 5)).toString().toUShort();
        quint16 vic_logicAddr = ECU_List_model->data(ECU_List_model->index(curIndex.row(), 1)).toString().toUShort();

        QString vic_logicAddr1 = ECU_List_model->data(ECU_List_model->index(curIndex.row(), 1)).toString();
        qDebug() << "vic_logicAddr1:" << vic_logicAddr1;

        vic_logicAddr = vic_logicAddr1.toUShort(NULL, 16);

        qDebug() << "ECU_List_model->data(ECU_List_model->index(curIndex.row(), 1)):" << ECU_List_model->data(ECU_List_model->index(curIndex.row(), 1));
        qDebug() << "ECU_List_model->data(ECU_List_model->index(curIndex.row(), 1)).toString():" << ECU_List_model->data(ECU_List_model->index(curIndex.row(), 1)).toString();
        qDebug() << "ECU_List_model->data(ECU_List_model->index(curIndex.row(), 1)).toUShort():" << QString::number(ECU_List_model->data(ECU_List_model->index(curIndex.row(), 1)).toString().toUShort(), 16);

        qDebug() << "vic_logicAddr1.toUShort():" << QString::number(vic_logicAddr1.toUShort(), 16);
        qDebug() << "IP:" << vic_IP << "Port:" << QString::number(port, 10) << "vic_logicAddr:" << QString::number(vic_logicAddr, 16);

        for(int i=0; i<vicInfoList.size(); i++)
        {
            VehicleInfo *node = (VehicleInfo *)&vicInfoList.at(i);
            if(node->addr == QHostAddress(vic_IP)
                && node->port == port
                && node->vic.logicalAddr == vic_logicAddr)
            {
                curVIC = node;
                break;
            }
        }

        if(curVIC == NULL)
        {
            qDebug() << "车辆信息未保存";
            return;
        }


        TcpData_Client->connectToHost(curVIC->addr, curVIC->port);

        if(true == TcpData_Client->waitForConnected(3000))
        {
            connect(TcpData_Client, &QTcpSocket::readyRead, this, &MainWindow::TcpDataReadPendingDatagrams);
            doipPacket doipMsg;
            doipMsg.RoutingActivationRequst(ui->lineEdit_logicAddr->text().toShort(NULL, 16), 0);
            TcpData_Client->write(doipMsg.Data());
            TcpData_Client->flush();
            ui->textBrowser->append("TCP连接成功");
            ui->pushButton_vic_connect->setText("断开车辆");
            ui->pushButton->setEnabled(false);
        }
        else
        {
            ui->textBrowser->append("车辆连接失败" + curVIC->addr.toString() + ":" + QString::number(curVIC->port, 10));
        }


    }
}


void MainWindow::on_pushButton_readDID_clicked()
{
    doipPacket doipMsg;
    QByteArray uds;
    quint16 DID = 0;

    if(!vicIsConnect)
    {
        qDebug() << "车辆未连接！";
        return;
    }

    DID = ui->lineEdit_DID->text().toUShort(NULL, 16);
    qDebug() << "DID:" << QString::number(DID, 16);
    uds.append(0x22);
    uds.append(DID>>8 & 0xff);
    uds.append(DID>>0 & 0xff);

    doipMsg.DiagnosticMsg(ui->lineEdit_logicAddr->text().toShort(NULL, 16), curVIC->vic.logicalAddr, uds);

    qDebug() << "msg:" << doipMsg.Data().toHex(' ');
    TcpData_Client->write(doipMsg.Data());
    TcpData_Client->flush();
}

void MainWindow::on_pushButton_readDTC_clicked()
{
    doipPacket doipMsg;
    QByteArray uds;
    quint16 DTC_mask = 0;

    if(!vicIsConnect)
    {
        qDebug() << "车辆未连接！";
        return;
    }

    DTC_mask = ui->lineEdit_DTC->text().toUShort(NULL, 16);
    qDebug() << "DTC mask:" << QString::number(DTC_mask, 16);
    uds.append(0x19);
    uds.append(0x02);
    uds.append(DTC_mask);

    doipMsg.DiagnosticMsg(ui->lineEdit_logicAddr->text().toShort(NULL, 16), curVIC->vic.logicalAddr, uds);

    qDebug() << "msg:" << doipMsg.Data().toHex(' ');
    TcpData_Client->write(doipMsg.Data());
    TcpData_Client->flush();
}

