#ifndef UDS_H
#define UDS_H
#include <qglobal.h>
#include <QByteArray>
#include <QString>
#include <QList>

class udsMsg
{
public:
    struct udsRequst
    {
        quint8  SID;    // 服务ID
        quint8  SubID;  // 子功能ID
        quint8  DID;    // 数据ID
        QByteArray Dat;
    };

    struct udsResponse
    {
        bool    isPositiveResponse;
        quint8  SID;    // 服务ID
        quint8  SubID;  // 子功能ID
        quint8  DID;    // 数据ID
        quint8  NRC;    // 数据ID
        QByteArray Dat;
    };

    struct dtcInfo
    {
        quint8 DTCHighByte;
        quint8 DTCMiddleByte;
        quint8 DTCLowByte;
        quint8 StatusOfDTC;
        QString eCodeStr;
        QString description;    // 错误码的描述 如P0128 发动机温度异常
        QString state;          // 当前或者历史
    };

    struct udsDtc
    {
        quint8  SID;    // 服务ID
        quint8  SubID;  // 子功能ID
        quint8  mask;   // 掩码
        QList<struct dtcInfo> dtcList;
    };

public:
    udsMsg();
    udsMsg(QByteArray &uds);

    // 解析相关
    bool isPositiveResponse();
    bool isUdsResponse(struct udsResponse &udsResp);
    bool DTCAnalyze(struct udsDtc &uds);

private:
    QByteArray *_udsPacket = NULL;
};

#endif // UDS_H
