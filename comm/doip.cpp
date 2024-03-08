#include "doip.h"
#include <QByteArray>
#include <QString>
#include <QDebug>

doipPacket::doipPacket(quint16 payloadType, QByteArray &payload)
{
    _packet_is_ext = false;
    creatHeader(payloadType, payload.size());
    _packet->append(payload);
}

doipPacket::doipPacket()
{
    _packet_is_ext = false;
    _packet = new QByteArray();
}

doipPacket::doipPacket(QByteArray &arr)
{
    _packet_is_ext = true;
    _packet = &arr;
}

doipPacket::~doipPacket()
{
    if(!isUnchangeable())
        delete _packet;
}

bool doipPacket::isUnchangeable()
{
    if(_packet_is_ext)
    {
        return true;
    }

    return false;
}

bool doipPacket::creatHeader(quint16 payloadType, quint32 payloadSize)
{
    if(isUnchangeable())
       return false;

    if(_packet)
        _packet->clear();
    else
        _packet = new QByteArray();

    // DoIP Header
    // Version ~Versison payloadType payloadSize
    // 1B      1B        2B          4B
    _packet->append(0x02);
    _packet->append(0xFD);
    _packet->append(payloadType >> 8 & 0xff);
    _packet->append(payloadType >> 0 & 0xff);

    _packet->append(payloadSize >> 24 & 0xff);
    _packet->append(payloadSize >> 16 & 0xff);
    _packet->append(payloadSize >> 8 & 0xff);
    _packet->append(payloadSize >> 0 & 0xff);

    return true;
}

QByteArray& doipPacket::Data(void)
{
    return *this->_packet;
}

bool doipPacket::VehicleAnnouncement(QString VIN, quint16 logicalAddr,
                                            QByteArray EID, QByteArray GID,
                                            quint8 Fur, quint8 syncSta)
{
    quint16 payloadType = 0x0004;
    quint32 payloadSize = 33;


    if(!creatHeader(payloadType, payloadSize))
        return false;

    // VehicleAnnouncement
    // VIN logicalAddr EID GID Fur syncSta
    // 17B     2B       6B  6B  1B  1B
    _packet->append(VIN.toUtf8());
    _packet->append(logicalAddr >> 8 & 0xff);
    _packet->append(logicalAddr >> 0 & 0xff);
    _packet->append(EID);
    _packet->append(GID);
    _packet->append(Fur);
    _packet->append(syncSta);

    return true;
}

bool doipPacket::RoutingActivationRequst(quint16 sourceAddr, quint8 activationType)
{
    quint16 payloadType = 0x0005;
    quint32 payloadSize = 11;

    if(!creatHeader(payloadType, payloadSize))
        return false;

    // RoutingActivationRequst
    // sourceAddr activationType ISO OEM
    //     2B     1B             4B  4B
    _packet->append(sourceAddr >> 8 & 0xff);
    _packet->append(sourceAddr >> 0 & 0xff);
    _packet->append(activationType);
    _packet->append(QByteArray::fromHex("00000000"));
    _packet->append(QByteArray::fromHex("00000000"));

    return true;
}

bool doipPacket::RoutingActivationResponse(quint16 testerLogicalAddr, quint16 sourceAddr, quint8 respCode)
{
    quint16 payloadType = 0x0006;
    quint32 payloadSize = 13;

    if(!creatHeader(payloadType, payloadSize))
        return false;

    // RoutingActivationResponse
    // testerLogicalAddr sourceAddr respCode ISO OEM
    //     2B               2B          1B    4B  4B
    _packet->append(testerLogicalAddr >> 8 & 0xff);
    _packet->append(testerLogicalAddr >> 0 & 0xff);
    _packet->append(sourceAddr >> 8 & 0xff);
    _packet->append(sourceAddr >> 0 & 0xff);
    _packet->append(respCode);
    _packet->append(QByteArray::fromHex("00000000"));
    _packet->append(QByteArray::fromHex("00000000"));

    return true;
}


bool doipPacket::isRoutingActivationRequst()
{
    quint16 payloadType;
    quint32 payloadSize;

    if(!_getDoipHeader(payloadType, payloadSize))
        return false;


    if(payloadType != 0x0005)
        return false;

    return true;
}

bool doipPacket::getSourceAddrFromRoutingActivationRequst(quint16 &sourceAddr)
{
    if(!isRoutingActivationRequst())
        return false;

    // RoutingActivationRequst
    // sourceAddr activationType ISO OEM
    //     2B     1B             4B  4B
    sourceAddr = _packet->at(8) << 8 | (_packet->at(9) & 0xff);
    qDebug() << "sourceAddr:0x" << QString::number(sourceAddr, 16);
    return true;
}

bool doipPacket::isRoutingActivationResponse()
{
    quint16 payloadType;
    quint32 payloadSize;

    if(!_getDoipHeader(payloadType, payloadSize))
        return false;

    if(payloadType != 0x0006)
        return false;

    return true;
}

bool doipPacket::RoutingActivationResponseAnalyze(quint16 &testerLogicalAddr, quint16 &sourceAddr, quint8 &respCode)
{
    if(!isRoutingActivationResponse())
        return false;

    // RoutingActivationResponse
    // testerLogicalAddr sourceAddr respCode ISO OEM
    //     2B               2B          1B    4B  4B
    testerLogicalAddr = _packet->at(8) << 8 | _packet->at(9);
    sourceAddr = _packet->at(10) << 8 | _packet->at(11);
    respCode = _packet->at(12);

    return true;
}

bool doipPacket::isDoipPacket()
{
    return false;
}

bool doipPacket::_getDoipHeader(quint16 &payloadType, quint32 &payloadSize)
{
    // DoIP Header
    // Version ~Versison payloadType payloadSize
    // 1B      1B        2B          4B

    if(_packet->size() < 8)
        return false;

    payloadType = _packet->at(2) << 8 | _packet->at(3);
    payloadSize = _packet->at(4) << 24 | _packet->at(5) << 16 | _packet->at(6) << 8 | _packet->at(7);

    return true;
}

bool doipPacket::DiagnosticMsg(quint16 sourceAddr, quint16 targetAddr, QByteArray &udsData)
{
    if(!creatHeader(0x8001, udsData.size() + 4))
        return false;

    _packet->append(sourceAddr >> 8 & 0xff);
    _packet->append(sourceAddr >> 0 & 0xff);
    _packet->append(targetAddr >> 8 & 0xff);
    _packet->append(targetAddr >> 0 & 0xff);

    _packet->append(udsData);

    return true;
}

bool doipPacket::DiagnosticMsgACKorNACK(quint16 sourceAddr, quint16 targetAddr, quint8 type, quint8 code, QByteArray &udsData)
{
    quint16 payloadType;
    quint32 payloadSize;


    if(type == doipPacket::ACK)
        payloadType = 0x8002;
    else
        payloadType = 0x8003;

    payloadSize = udsData.size() + 5;
    if(!creatHeader(payloadType, payloadSize))
        return false;

    _packet->append(sourceAddr >> 8 & 0xff);
    _packet->append(sourceAddr >> 0 & 0xff);
    _packet->append(targetAddr >> 8 & 0xff);
    _packet->append(targetAddr >> 0 & 0xff);
    _packet->append(code);

    if(udsData.size() != 0)
        _packet->append(udsData);

    return true;
}

bool doipPacket::DiagnosticMsgACK(quint16 sourceAddr, quint16 targetAddr, quint8 code)
{
    QByteArray tmp;
    tmp.clear();
    return DiagnosticMsgACKorNACK(sourceAddr, targetAddr, doipPacket::ACK, code, tmp);
}

bool doipPacket::DiagnosticMsgACK(quint16 sourceAddr, quint16 targetAddr, quint8 code, QByteArray &udsData)
{
    return DiagnosticMsgACKorNACK(sourceAddr, targetAddr, doipPacket::ACK, code, udsData);
}

/**
 * @brief doipPacket::DiagnosticMsgNACK
 * @param sourceAddr
 * @param targetAddr
 * @param code
 *          0x00, 0x01 为ISO13400保留
 *          0x02: 无效的源地址
 *          0x03: 无效的目的地址
 *          0x04: 诊断消息长度太大
 *          0x05: 超出内存范围
 *          0x06: 目的端口不可达
 *          0x07: 未知网络
 *          0x08: 传输层错误
 * @return
 */
bool doipPacket::DiagnosticMsgNACK(quint16 sourceAddr, quint16 targetAddr, quint8 code)
{
    QByteArray tmp;
    tmp.clear();
    return DiagnosticMsgACKorNACK(sourceAddr, targetAddr, doipPacket::NACK, code, tmp);
}

bool doipPacket::DiagnosticMsgNACK(quint16 sourceAddr, quint16 targetAddr, quint8 code, QByteArray &udsData)
{
    return DiagnosticMsgACKorNACK(sourceAddr, targetAddr, doipPacket::NACK, code, udsData);
}

bool doipPacket::isDiagnosticMsg()
{
    quint16 payloadType;
    quint32 payloadSize;

    if(!_getDoipHeader(payloadType, payloadSize))
        return false;

    if((payloadType != 0x8001)
        && (payloadType != 0x8002)
        && (payloadType != 0x8003))
        return false;

    return true;
}

bool doipPacket::DiagnosticMsgAnalyze(struct DiagnosticMsg &diagInfo)
{
    if(!isDiagnosticMsg())
        return false;

    quint16 payloadType = _packet->at(2) << 8 | _packet->at(3);
    quint32 start_index = 0;

    if(payloadType == 0x8001)
    {
        diagInfo.type = MSG;
        start_index = 12;
    }
    else if(payloadType == 0x8002)
    {
        diagInfo.type = ACK;
        diagInfo.code = (_packet->at(12) & 0xff);
        start_index = 13;
    }
    else if(payloadType == 0x8003)
    {
        diagInfo.type = NACK;
        diagInfo.code = (_packet->at(12) & 0xff);
        start_index = 13;
    }
    else
    {
        qDebug() << "UNKNOWN type:0x" << QString::number(payloadType, 16);
        diagInfo.type = UNKNOWN;
        return false;
    }

    diagInfo.sourceAddr = _packet->at(8) << 8 | (_packet->at(9) & 0xff);
    diagInfo.targetAddr = _packet->at(10) << 8 | (_packet->at(11) & 0xff);

    diagInfo.udsData.clear();
    diagInfo.udsData = _packet->mid(start_index, _packet->size()-start_index);  // TODO: 拷贝数据

    return true;
}

bool doipPacket::VehicleIdentificationRequest()
{
    quint16 payloadType = 0x0001;
    quint32 payloadSize = 0;

    if(!creatHeader(payloadType, payloadSize))
        return false;

    return true;
}

bool doipPacket::VehicleIdentificationRequest(QString VIN)
{
    quint16 payloadType = 0x0003;
    quint32 payloadSize = 17;

    if(!creatHeader(payloadType, payloadSize))
        return false;

    _packet->append(VIN.mid(0, payloadSize).toUtf8());

    return true;
}

bool doipPacket::VehicleIdentificationRequest(QByteArray EID)
{
    quint16 payloadType = 0x0002;
    quint32 payloadSize = 6;

    if(!creatHeader(payloadType, payloadSize))
        return false;

    _packet->append(EID.mid(0, payloadSize));

    return true;
}

bool doipPacket::isVehicleIdentificationRequest()
{
    quint16 payloadType;
    quint32 payloadSize;

    if(!_getDoipHeader(payloadType, payloadSize))
        return false;

    if(payloadType != 0x0001 && payloadType != 0x0002 && payloadType != 0x0003)
        return false;

    return true;
}

bool doipPacket::VehicleIdentificationRequestAnalyze(QString &VIN, QByteArray &EID)
{
    if(!isVehicleIdentificationRequest())
        return false;

    if(_packet->size() <= DoIPHeaderLen)
        return false;

    quint16 payloadType;
    quint32 payloadSize;

    if(!_getDoipHeader(payloadType, payloadSize))
        return false;

    if(payloadType == 0x0002)
        VIN = _packet->mid(DoIPHeaderLen, 17);
    else if(payloadType == 0x0003)
        EID = _packet->mid(DoIPHeaderLen, 6);
    else
        return false;

    return true;
}

bool doipPacket::VehicleAnnouncementAnalyze(struct VehicleAnnouncement &vic)
{
    quint16 payloadType;
    quint32 payloadSize;

    if(!_getDoipHeader(payloadType, payloadSize))
        return false;

    if(payloadType != 0x0004)
        return false;

    vic.VIN = _packet->mid(DoIPHeaderLen, 17);
    vic.logicalAddr = _packet->at(25) << 8 | (_packet->at(26) & 0xff);
    vic.EID = _packet->mid(27, 6);
    vic.GID = _packet->mid(33, 6);
    vic.Fur = _packet->mid(39, 1).toInt();
    vic.syncSta = _packet->mid(40, 1).toInt();
    return true;
}

