#include "doip.h"
#include <QByteArray>
#include <QString>
#include <QDebug>

doipPacket::doipPacket(quint16 payloadType, QByteArray &payload)
{
    _packet = new QByteArray();
    _packet->append(0x02);
    _packet->append(0xFD);
    _packet->append(payloadType >> 8 & 0xff);
    _packet->append(payloadType >> 0 & 0xff);

    _packet->append(payload.size() >> 24 & 0xff);
    _packet->append(payload.size() >> 16 & 0xff);
    _packet->append(payload.size() >> 8 & 0xff);
    _packet->append(payload.size() >> 0 & 0xff);

    _packet->append(payload);
}

doipPacket::doipPacket()
{
    _packet = new QByteArray();
}

QByteArray& doipPacket::Data(void)
{
    return *this->_packet;
}

QByteArray& doipPacket::VehicleAnnouncement(QString VIN, quint16 logicalAddr,
                                            QByteArray EID, QByteArray GID,
                                            quint8 Fur, quint8 syncSta)
{
    _packet->clear();

    quint16 payloadType = 0x0004;
    quint32 payloadSize = 33;

    // DoIP Header
    _packet->append(0x02);
    _packet->append(0xFD);
    _packet->append(payloadType >> 8 & 0xff);
    _packet->append(payloadType >> 0 & 0xff);

    _packet->append(payloadSize >> 24 & 0xff);
    _packet->append(payloadSize >> 16 & 0xff);
    _packet->append(payloadSize >> 8 & 0xff);
    _packet->append(payloadSize >> 0 & 0xff);

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

    return *this->_packet;
}

QByteArray& doipPacket::RoutingActivationRequst(quint16 sourceAddr, quint8 activationType)
{
    _packet->clear();

    quint16 payloadType = 0x0005;
    quint32 payloadSize = 11;

    // DoIP Header
    _packet->append(0x02);
    _packet->append(0xFD);
    _packet->append(payloadType >> 8 & 0xff);
    _packet->append(payloadType >> 0 & 0xff);

    _packet->append(payloadSize >> 24 & 0xff);
    _packet->append(payloadSize >> 16 & 0xff);
    _packet->append(payloadSize >> 8 & 0xff);
    _packet->append(payloadSize >> 0 & 0xff);

    // RoutingActivationRequst
    // sourceAddr activationType ISO OEM
    //     2B     1B             4B  4B
    _packet->append(sourceAddr >> 8 & 0xff);
    _packet->append(sourceAddr >> 0 & 0xff);
    _packet->append(activationType);
    _packet->append(QByteArray::fromHex("00000000"));
    _packet->append(QByteArray::fromHex("00000000"));

    return *this->_packet;
}

QByteArray& doipPacket::RoutingActivationResponse(quint16 testerLogicalAddr, quint16 sourceAddr, quint8 respCode)
{
    _packet->clear();

    quint16 payloadType = 0x0006;
    quint32 payloadSize = 13;

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

    return *this->_packet;
}

bool doipPacket::isDoipPacket(QByteArray &arr)
{
    return false;
}

bool doipPacket::_getDoipHeader(QByteArray &arr, quint16 &payloadType, quint32 &payloadSize)
{
    // DoIP Header
    // Version ~Versison payloadType payloadSize
    // 1B      1B        2B          4B

    if(arr.size() < 8)
        return false;

    payloadType = arr.at(2) << 8 | arr.at(3);
    payloadSize = arr.at(4) << 24 | arr.at(5) << 16 | arr.at(6) << 8 | arr.at(7);

    qDebug() << "payloadType:0x" << QString::number(payloadType, 16) << "payloadSize:" << payloadSize;

    return true;
}

bool doipPacket::isRoutingActivationRequst(QByteArray &arr)
{
    quint16 payloadType;
    quint32 payloadSize;

    if(!_getDoipHeader(arr, payloadType, payloadSize))
        return false;


    if(payloadType != 0x0005)
        return false;

    return true;
}

bool doipPacket::getSourceAddrFromRoutingActivationRequst(QByteArray &arr, quint16 &sourceAddr)
{
    if(!isRoutingActivationRequst(arr))
        return false;

    // RoutingActivationRequst
    // sourceAddr activationType ISO OEM
    //     2B     1B             4B  4B
    sourceAddr = arr.at(8) << 8 | (arr.at(9) & 0xff);
    qDebug() << "sourceAddr:0x" << QString::number(sourceAddr, 16);
    return true;
}

bool doipPacket::isRoutingActivationResponse(QByteArray &arr)
{
    quint16 payloadType;
    quint32 payloadSize;

    if(!_getDoipHeader(arr, payloadType, payloadSize))
        return false;

    if(payloadType != 0x0006)
        return false;

    return false;
}


doipPacket::~doipPacket()
{
    delete _packet;
}

