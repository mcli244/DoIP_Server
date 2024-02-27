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
        NACK,
        MSG,
        UNKNOWN
    };

    struct DiagnosticMsg
    {
        enum DiagnosticMsgType type;
        quint16 sourceAddr;
        quint16 targetAddr;
        quint8  code;   // ACKorNACK code
        QByteArray udsData;
    };

    // 外部传进来数据，不可改动，仅能解析
    doipPacket(QByteArray &arr);
    bool isUnchangeable();

    // 内部创造数据，可以改动，创造
    doipPacket(quint16 payloadType, QByteArray &payload);
    doipPacket();
    ~doipPacket();

    bool creatHeader(quint16 payloadType, quint32 payloadSize);

    QByteArray& Data(void);
    bool VehicleAnnouncement(QString VIN, quint16 logicalAddr,
                                    QByteArray EID, QByteArray GID,
                                    quint8 Fur, quint8 syncSta);
    bool RoutingActivationRequst(quint16 sourceAddr, quint8 activationType);
    bool RoutingActivationResponse(quint16 testerLogicalAddr, quint16 sourceAddr, quint8 respCode);

    bool isDiagnosticMsg();
    bool DiagnosticMsg(quint16 sourceAddr, quint16 targetAddr, QByteArray &udsData);
    bool DiagnosticMsgACK(quint16 sourceAddr, quint16 targetAddr, quint8 code);
    bool DiagnosticMsgACK(quint16 sourceAddr, quint16 targetAddr, quint8 code, QByteArray &udsData);
    bool DiagnosticMsgNACK(quint16 sourceAddr, quint16 targetAddr, quint8 code);
    bool DiagnosticMsgNACK(quint16 sourceAddr, quint16 targetAddr, quint8 code, QByteArray &udsData);
    bool DiagnosticMsgAnalyze(struct DiagnosticMsg &diagInfo);

    bool isDoipPacket();
    bool isRoutingActivationRequst();
    bool isRoutingActivationResponse();
    bool getSourceAddrFromRoutingActivationRequst(quint16 &sourceAddr);

  private:
    bool _getDoipHeader(quint16 &payloadType, quint32 &payloadSize);
    bool DiagnosticMsgACKorNACK(quint16 sourceAddr, quint16 targetAddr, quint8 type, quint8 code, QByteArray &udsData);
    QByteArray *_packet = NULL;
    bool _packet_is_ext = true;
};


#endif // DOIP_H
