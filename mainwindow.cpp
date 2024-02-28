#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkInterface>
#include <QDebug>
#include <QtEndian>
#include <QTimer>
#include <QStandardItemModel>
#include <QThread>
#include <QItemDelegate>
#include <QStyleOptionViewItem>

class ReadOnlyDelegate: public QItemDelegate
{

public:
    ReadOnlyDelegate(QWidget *parent = NULL):QItemDelegate(parent)
    {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
const QModelIndex &index) const override //final
    {
        Q_UNUSED(parent)
        Q_UNUSED(option)
        Q_UNUSED(index)
        return NULL;
    }
};

class UserIDDelegate : public QItemDelegate
{

public:
    UserIDDelegate(QObject* parent = 0) : QItemDelegate(parent) { }
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
        const QModelIndex& index) const
    {
        QLineEdit* editor = new QLineEdit(parent);
        QRegExp regExp("^[PCBU]\\d{4}$");
        editor->setValidator(new QRegExpValidator(regExp, parent));
        return editor;
    }
    void setEditorData(QWidget* editor, const QModelIndex& index) const
    {
        QString text = index.model()->data(index, Qt::EditRole).toString();
        QLineEdit* lineEdit = static_cast<QLineEdit*>(editor);
        lineEdit->setText(text);
    }
    void setModelData(QWidget* editor, QAbstractItemModel* model,
        const QModelIndex& index) const
    {
        QLineEdit* lineEdit = static_cast<QLineEdit*>(editor);
        QString text = lineEdit->text();
        model->setData(index, text, Qt::EditRole);
    }
    void updateEditorGeometry(QWidget* editor,
        const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        editor->setGeometry(option.rect);
    }
};


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label_state->setText("DoIP Server is stop");

    // 禁用 车辆声明广播按钮
    ui->pushButton->setEnabled(false);


    initDID();
    initDTC();

    // DOIP报文测试
    doipPacket doipPkg;

    doipPkg.RoutingActivationRequst(0x0e01, 1);
    qDebug() << "DOIP RoutingActivationRequst Packet:" << doipPkg.Data().toHex(' ');

    doipPkg.DiagnosticMsgACK(0x0e01, 0x0DFF, 0x00);
    qDebug() << "DOIP DiagnosticMsgACK Packet:" << doipPkg.Data().toHex(' ');

    doipPkg.DiagnosticMsgNACK(0x0e01, 0x0DFF, 0x02);
    qDebug() << "DOIP DiagnosticMsgNACK Packet:" << doipPkg.Data().toHex(' ');

    QByteArray udsData = QByteArray::fromHex("1001");
    doipPkg.DiagnosticMsg(0x0e01, 0x0DFF, udsData);
    qDebug() << "DOIP DiagnosticMsg Packet:" << doipPkg.Data().toHex(' ');

    struct doipPacket::DiagnosticMsg dmsg;
    doipPkg.DiagnosticMsgAnalyze(dmsg);
    qDebug() << "DOIP DiagnosticMsgAnalyze "
             << "type:" << QString::number(dmsg.type)
             << "sourceAddr:" << QString::number(dmsg.sourceAddr, 16)
             << "targetAddr:" << QString::number(dmsg.targetAddr, 16)
             << "uds:" << dmsg.udsData.toHex(' ');

    iface_refresh();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::initDID(void)
{
    // ECU DID
    // 创建一个标准的表格模型
    ECU_DID_model = new QStandardItemModel(0,3);
    ECU_DID_model->setHorizontalHeaderLabels(QStringList() << "DID" << "值" << "描述");

    ECU_DID_model->setItem(0, 0, new QStandardItem("0xF190"));
    ECU_DID_model->setItem(0, 1, new QStandardItem(ui->lineEdit_VIN->text()));
    ECU_DID_model->setItem(0, 2, new QStandardItem("标识VIN码"));

    ui->tableView_ECU_DID->setModel(ECU_DID_model);
    ui->tableView_ECU_DID->resizeColumnsToContents();
    ui->tableView_ECU_DID->show();
}

bool MainWindow::DTCToBytes(QString dtc, quint8 &DTCHighByte, quint8 &DTCMiddleByte)
{
    quint8 tmp = 0;

    if(dtc.size() < 5)  return false;

    switch(dtc.at(0).toLatin1())
    {
        case 'P':   tmp = 0; break;
        case 'C':   tmp = 1; break;
        case 'B':   tmp = 2; break;
        case 'U':   tmp = 3; break;
        default: return false;
    }

    DTCHighByte = 0;
    DTCHighByte |= tmp << 6;

    tmp = dtc.mid(1, 1).toInt() & 0xff;
    DTCHighByte |= tmp << 4;

    tmp = dtc.mid(2, 1).toInt() & 0xff;
    DTCHighByte |= tmp;

    tmp = dtc.mid(3, 2).toInt(NULL, 16) & 0xff;
    DTCMiddleByte = tmp;

//    qDebug() << "DTCHighByte:0x" << QString::number(DTCHighByte, 16)
//             << "DTCMiddleByte:0x" << QString::number(DTCMiddleByte, 16);

    return true;
}

void MainWindow::initDTC(void)
{
    /* DTC故障码描述
            |  DTCHighByte(Hex)        |DTCMiddleByte(Hex)| DTCLowByte(Hex) | StatusOfDTC(Hex)|
      BIT   |15 14 | 13 12 | 11 10 9 8 | 7 6 5 4 3 2 1 0  | 7 6 5 4 3 2 1 0 | 7 6 5 4 3 2 1 0 |

      DTCHighByte(15:8):
          bit[15:14]: 一位字符
            00: 'P'     Powertrain 动力系统故障
            01: 'C'     Chassis 底盘故障
            10: 'B'     Body 车身故障
            11: 'U'     Network 网络故障

          bit[13:12]: 一位字符
            00: '0'     ISO/SAE 标准定义的故障码
            01: '1'     制造商自定义的故障码
            10: '2'     ISO/SAE 标准定义的故障码
            11: '3'     ISO/SAE 保留

          bit[11:8]: 一位字符 （只有0-8）
            0000: “0”表示燃油和空气计量辅助排放控制整个系统
            0001: “1”表示燃油和空气计量系统；
            0010: “2”表示燃油和空气计量系统(喷油器)；
            0011: “3”表示点火系统；
            0100: “4”表示废气控制系统；
            0101: “5”表示巡航、怠速控制系统；
            0110: “6”表示与控制单元相关；
            0111: "7"表示变速箱系统等。
            1000: “8”表示变速箱系统等。
            其他： 保留
      DTCMiddleByte(7:0):
        表示具体故障对象和类型。

    */

    // ECU DTC
    // 创建一个标准的表格模型 P0128 - 0000
    quint8 DTCHighByte;
    quint8 DTCMiddleByte;
    QString code = "P0128";

    DTCToBytes(code, DTCHighByte, DTCMiddleByte);

    ECU_DTC_model = new QStandardItemModel(0,6);
    ECU_DTC_model->setHorizontalHeaderLabels(QStringList()
                                             << "五位标准故障码"
                                             << "DTCHighByte(Hex)"
                                             << "DTCMiddleByte(Hex)"
                                             << "DTCLowByte(Hex)"
                                             << "StatusOfDTC(Hex)"
                                             << "描述");
    ECU_DTC_model->setItem(0, 0, new QStandardItem(code));
    ECU_DTC_model->setItem(0, 1, new QStandardItem(QString::asprintf("0x%02X", DTCHighByte)));
    ECU_DTC_model->setItem(0, 2, new QStandardItem(QString::asprintf("0x%02X", DTCMiddleByte)));
    ECU_DTC_model->setItem(0, 3, new QStandardItem("0x00"));
    ECU_DTC_model->setItem(0, 4, new QStandardItem("0x00"));
    ECU_DTC_model->setItem(0, 5, new QStandardItem("发动机温度过低"));

    //设置某列只读
    ReadOnlyDelegate* readOnlyDelegate = new ReadOnlyDelegate();
    ui->tableView_ECU_DTC->setItemDelegateForColumn(1, readOnlyDelegate);
    ui->tableView_ECU_DTC->setItemDelegateForColumn(2, readOnlyDelegate);

    // 只允许输入以PCBU字符开头，后面跟4个数字的数据
    UserIDDelegate* UserID = new UserIDDelegate();
    ui->tableView_ECU_DTC->setItemDelegateForColumn(0, UserID);

    ui->tableView_ECU_DTC->setModel(ECU_DTC_model);
    ui->tableView_ECU_DTC->resizeColumnsToContents();
    ui->tableView_ECU_DTC->show();

    connect(ECU_DTC_model, &QStandardItemModel::itemChanged, [=](){
        // 当标准错误码改变后，刷新DTCHighByte和DTCMidelByte
        QModelIndex index = ui->tableView_ECU_DTC->currentIndex();
        if(index.column() != 0)
            return;

        QString Code = ECU_DTC_model->data(ECU_DTC_model->index(index.row(), 0)).toString().trimmed();  // 去掉前面的空格

        qDebug() << "index:" << index.row() << index.column();

        quint8 DTCHighByte;
        quint8 DTCMiddleByte;

        ui->tableView_ECU_DTC->setCurrentIndex(ECU_DTC_model->index(index.row(), 3));   // 这个很重要 且放在setItem之前 这样这个信号就只会触发两次
        if(DTCToBytes(Code, DTCHighByte, DTCMiddleByte))
        {
            // TODO：这里可以优化，因为每次setItem,都将触发QStandardItemModel::itemChanged信号，这里会进入三次这个处理函数
            ECU_DTC_model->setItem(index.row(), 0, new QStandardItem(Code));
            ECU_DTC_model->setItem(index.row(), 1, new QStandardItem(QString::asprintf("0x%02X", DTCHighByte)));
            ECU_DTC_model->setItem(index.row(), 2, new QStandardItem(QString::asprintf("0x%02X", DTCMiddleByte)));
        }
    });
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

bool MainWindow::getValueByDID(quint32 did, QByteArray &value)
{
    // 查询tabview里面的信息
    quint32 did_tmp = 0;
    for(int i=0; i<ECU_DID_model->rowCount(); i++)
    {
        did_tmp = ECU_DID_model->data(ECU_DID_model->index(i, 0)).toString().toInt(NULL, 16);
//        qDebug() << "rowCount:" << QString::number(ECU_DID_model->rowCount())
//                 << "did:0x" << QString::number(did, 16)
//                 << "did_tmp:0x" << QString::number(did_tmp, 16)
//                 << "值:" << ECU_DID_model->data(ECU_DID_model->index(i, 1)).toString()
//                 << "描述:" << ECU_DID_model->data(ECU_DID_model->index(i, 2)).toString();
        if(did_tmp == did)
        {
            value = ECU_DID_model->data(ECU_DID_model->index(i, 1)).toByteArray();
            return true;
        }
    }

    return false;
}

bool MainWindow::DiagnosticMsgPro(QByteArray &uds, QByteArray &resp)
{
    if(!uds.size())
        return false;

    quint8 sid = uds.at(0);
    quint32 did;
    QByteArray value;

    switch (sid) {
    case 0x22:
        // 获取DID
        if(uds.size() > 3)
        {
            did = (uds.at(1) & 0xff) << 24;
            did |= (uds.at(2) & 0xff) << 16;
            did |= (uds.at(3) & 0xff) << 8;
            did |= (uds.at(4) & 0xff) << 0;
        }
        else
        {
            did = (uds.at(1) & 0xff) << 8;
            did |= (uds.at(2) & 0xff) << 0;
        }

        if(getValueByDID(did, value))
        {
            resp.append(0x62);
            resp.append(uds.mid(1, uds.size() > 3 ? 4:2));
            resp.append(value);
        }
        else
        {
            resp.append(0x7f);
            resp.append(sid);
            resp.append(doipPacket::requestOutOfRange);
        }
        break;
    default:
        resp.append(0x7f);
        resp.append(sid);
        resp.append(doipPacket::ServiceNotSupported);  // sid不支持
        break;
    }

    return true;
}

void MainWindow::DiagnosticMsgPro(struct doipPacket::DiagnosticMsg &dmsg, QByteArray &resp)
{
    switch (dmsg.type) {
    case doipPacket::ACK:
        break;
    case doipPacket::NACK:
        break;
    case doipPacket::MSG:

        DiagnosticMsgPro(dmsg.udsData, resp);
        break;
    default:
        break;
    }
}

void MainWindow::TcpDataReadPendingDatagrams()
{
    //接收数据
    QByteArray arr = TcpData_Client->readAll();
    qDebug()<<"IP:" <<TcpData_Client_addr.toString() << " Port:" << TcpData_Client_port <<"recv:" << arr.toHex(' ');

    doipPacket doipMsg(arr);
    quint16 testerAddr;

    if(doipMsg.isRoutingActivationRequst())
    {
        if(doipMsg.getSourceAddrFromRoutingActivationRequst(testerAddr))
        {
            doipPacket doipRes;
            if(doipRes.RoutingActivationResponse(testerAddr, ui->lineEdit_logicAddr->text().toShort(NULL, 16), 0x10))
            {
                TcpData_Client->write(doipRes.Data());

                qDebug() << "testrAddr:" << QString::number(testerAddr, 16) << "RoutingActivationResponse:" << doipRes.Data().toHex(' ');
                ui->textBrowser->append("testrAddr:0x" + QString::number(testerAddr, 16) + " RoutingActivationResponse:" + doipRes.Data().toHex(' '));
                ui->label_state->setText("RoutingActivationResponse suncc");
            }
            else
                qDebug() << "creat RoutingActivationResponse pkg filed";
        }
        else
        {
            qDebug() << "getSourceAddrFromRoutingActivationRequst failed";
            ui->textBrowser->append("getSourceAddrFromRoutingActivationRequst failed");
            ui->label_state->setText("RoutingActivationResponse failed");
        }
    } else if(doipMsg.isDiagnosticMsg())
    {
        struct doipPacket::DiagnosticMsg dmsg;
        if(doipMsg.DiagnosticMsgAnalyze(dmsg))
        {
            QByteArray resp;
            QString result = QString("DOIP DiagnosticMsgAnalyze type: " + QString::number(dmsg.type)
                             + " sourceAddr: " + QString::number(dmsg.sourceAddr, 16)
                             + " targetAddr: " + QString::number(dmsg.targetAddr, 16)
                             + " uds:" + dmsg.udsData.toHex(' '));
            qDebug() << result;
            ui->textBrowser->append(result);

            doipPacket doipResACK;

            // 回应DOIP ACK
            if(doipResACK.DiagnosticMsgACK(ui->lineEdit_logicAddr->text().toShort(NULL, 16), dmsg.sourceAddr, 0x00))
            {
                qDebug() << "DiagnosticMsgACK:" << doipResACK.Data().toHex(' ');
                TcpData_Client->write(doipResACK.Data());
                TcpData_Client->flush();    //将缓冲区的数据全部发送出去，不然这个的ACK和下面的MSG包会黏在一起发出去
            }

            // 回应 MSG
            doipPacket doipRes;
            DiagnosticMsgPro(dmsg, resp);
            if(doipRes.DiagnosticMsg(ui->lineEdit_logicAddr->text().toShort(NULL, 16), dmsg.sourceAddr,  resp))
            {
                TcpData_Client->write(doipRes.Data());

                qDebug() << "testrAddr:" << ui->lineEdit_logicAddr->text() << "DiagnosticMsg:" << doipRes.Data().toHex(' ');
                ui->textBrowser->append("testrAddr:0x" + ui->lineEdit_logicAddr->text() + " DiagnosticMsg:" + doipRes.Data().toHex(' '));
                ui->label_state->setText("RoutingActivationResponse suncc");
            }
            else
                qDebug() << "creat RoutingActivationResponse pkg filed";
        }
        else
            qDebug() << "DiagnosticMsgAnalyze failed";

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

    qDebug()<<"IP:" <<TcpData_Client_addr.toString() << " Port:" << QString::number(TcpData_Client_port) <<"连接上来!";

    ui->textBrowser->append("IP:"+TcpData_Client_addr.toString() + " Port:" + QString::number(TcpData_Client_port) + "connected!");

    ui->label_state->setText(QString("new client IP:"+TcpData_Client_addr.toString() + " Port:" + QString::number(TcpData_Client_port)));
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

        // 启动定时器，间隔500ms发送车辆声明
        Discover_timer = new QTimer();
        Discover_timer->setInterval(500);
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


void MainWindow::on_pushButton_add_col_clicked()
{

}


void MainWindow::on_pushButton_add_row_clicked()
{

    QList<QStandardItem *> aitemlist;
    QStandardItem *item;
    for(int i=0;i<ECU_DID_model->columnCount()-1;i++){
        item=new QStandardItem("0");
        aitemlist<<item;
    }
    QString str=ECU_DID_model->headerData(ECU_DID_model->columnCount()-1,Qt::Horizontal).toString();
    item=new QStandardItem(str);
//    item->setCheckable(true);
//    item->setBackground(QBrush(Qt::red));
    aitemlist<<item;
    ECU_DID_model->insertRow(ECU_DID_model->rowCount(),aitemlist);
}


void MainWindow::on_pushButton_del_col_clicked()
{

}


void MainWindow::on_pushButton_del_row_clicked()
{
    QModelIndex curindex = ui->tableView_ECU_DID->currentIndex();
    ECU_DID_model->removeRow(curindex.row());
}


void MainWindow::on_pushButton_ECU_DID_Refresh_clicked()
{

}


void MainWindow::on_pushButton_DTC_add_row_clicked()
{
    QList<QStandardItem *> aitemlist;
    QStandardItem *item;
    for(int i=0;i<ECU_DTC_model->columnCount()-1;i++){
        item=new QStandardItem(" ");
        aitemlist<<item;
    }
    QString str=ECU_DTC_model->headerData(ECU_DTC_model->columnCount()-1,Qt::Horizontal).toString();
    item=new QStandardItem(str);
    aitemlist<<item;
    ECU_DTC_model->insertRow(ECU_DTC_model->rowCount(),aitemlist);
}


void MainWindow::on_pushButton_DTC_del_row_clicked()
{
    QModelIndex curindex = ui->tableView_ECU_DTC->currentIndex();
    ECU_DTC_model->removeRow(curindex.row());
}

