#ifndef DOIP_H
#define DOIP_H
#include <qglobal.h>
#include <QByteArray>
#include <QString>

class doipPacket
{
  public:
    enum DiagnosticMsgType {
        ACK,
        NACK
    };

    doipPacket(quint16 payloadType, QByteArray &payload);
    doipPacket();
    ~doipPacket();

    bool creatHeader(quint16 payloadType, quint32 payloadSize);

    QByteArray& Data(void);
    QByteArray& VehicleAnnouncement(QString VIN, quint16 logicalAddr,
                                    QByteArray EID, QByteArray GID,
                                    quint8 Fur, quint8 syncSta);
    QByteArray& RoutingActivationRequst(quint16 sourceAddr, quint8 activationType);
    QByteArray& RoutingActivationResponse(quint16 testerLogicalAddr, quint16 sourceAddr, quint8 respCode);

    QByteArray& DiagnosticMsg(quint16 sourceAddr, quint16 targetAddr, QByteArray &udsData);
    QByteArray& DiagnosticMsgACK(quint16 sourceAddr, quint16 targetAddr, quint8 code);
    QByteArray& DiagnosticMsgACK(quint16 sourceAddr, quint16 targetAddr, quint8 code, QByteArray &udsData);
    QByteArray& DiagnosticMsgNACK(quint16 sourceAddr, quint16 targetAddr, quint8 code);
    QByteArray& DiagnosticMsgNACK(quint16 sourceAddr, quint16 targetAddr, quint8 code, QByteArray &udsData);

    bool isDoipPacket(QByteArray &arr);
    bool isRoutingActivationRequst(QByteArray &arr);
    bool isRoutingActivationResponse(QByteArray &arr);
    bool getSourceAddrFromRoutingActivationRequst(QByteArray &arr, quint16 &sourceAddr);

  private:
    bool _getDoipHeader(QByteArray &arr, quint16 &payloadType, quint32 &payloadSize);
    QByteArray& DiagnosticMsgACKorNACK(quint16 sourceAddr, quint16 targetAddr, quint8 type, quint8 code, QByteArray &udsData);
    QByteArray *_packet;
};


#endif // DOIP_H
