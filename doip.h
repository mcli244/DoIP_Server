#ifndef DOIP_H
#define DOIP_H
#include <qglobal.h>
#include <QByteArray>
#include <QString>

class doipPacket
{
  public:
    doipPacket(quint16 payloadType, QByteArray &payload);
    doipPacket();
    ~doipPacket();

    QByteArray &Data(void);
    QByteArray &VehicleAnnouncement(QString VIN, quint16 logicalAddr,
                                    QByteArray EID, QByteArray GID,
                                    quint8 Fur, quint8 syncSta);
    QByteArray &RoutingActivationRequst(quint16 sourceAddr, quint8 activationType);
    QByteArray &RoutingActivationResponse(quint16 testerLogicalAddr, quint16 sourceAddr, quint8 respCode);

    bool isDoipPacket(QByteArray &arr);
    bool isRoutingActivationRequst(QByteArray &arr);
    bool isRoutingActivationResponse(QByteArray &arr);
    bool getSourceAddrFromRoutingActivationRequst(QByteArray &arr, quint16 &sourceAddr);

  private:
    bool _getDoipHeader(QByteArray &arr, quint16 &payloadType, quint32 &payloadSize);
    QByteArray *_packet;
};


#endif // DOIP_H
