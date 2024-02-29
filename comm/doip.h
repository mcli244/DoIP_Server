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

    enum UdsNRCType {
        ServiceNotSupported							= 0x11, //服务不支持，诊断仪发送的请求消息中服务标识符无法识别或不支持；
        SubFunctionNotSupported						= 0x12, //不支持子服务，诊断仪发送的请求消息中子服务无法识别或不支持；
        IncorrectMessageLengthOrInvalidFormat		= 0x13, //不正确的消息长度或无效的格式，请求消息长度与特定服务规定的长度不匹配或者是参数格式与特定服务规定的格式不匹配；
        BusyRepeatRequest							= 0x21, //重复请求忙，表明ECU太忙而不能去执行请求。一般来说，在这种情况下，诊断仪应进行重复请求工作；
        conditionsNotCorrect						= 0x22, //条件不正确，表明ECU的状态条件不允许支持该请求；
        requestSequenceError						= 0x24, //请求序列错误，表明收到的是非预期的请求消息序列；
        noResponseFromSubnetComponent				= 0x25, //子网节点无应答，表明ECU收到请求，但所请求的操作无法执行；
        failurePreventsExecutionOfRequestedAction	= 0x26, //故障阻值请求工作执行，表明请求的动作因一故障原因而没有执行；
        requestOutOfRange							= 0x31, //请求超出范围，请求消息包含一个超出允许范围的参数，或者是不支持的数据标识符					//例程标识符的访问；
        securityAccessDenied						= 0x33, //安全访问拒绝，诊断仪无法通过ECU的安全策略；
        invalidKey									= 0x35, //密钥无效，诊断仪发送的密钥与ECU内存中的密钥不匹配；
        exceedNumberOfAttempts						= 0x36, //超出尝试次数，诊断仪尝试获得安全访问失败次数超过了ECU安全策略允许的值；
        requiredTimeDelayNotExpired					= 0x37, //所需时间延迟未到，在ECU所需的请求延迟时间过去之前诊断仪又执行了一次请求；
        uploadDownloadNotAccepted					= 0x70, //不允许上传下载，表明试图向ECU内存上传					//下载数据失败的原因是条件不允许；
        transferDataSuspended						= 0x71, //数据传输暂停，表明由于错误导致数据传输操作的中止；
        generalProgrammingFailure					= 0x72, //一般编程失败，表明在不可擦除的内存设备中进行擦除或编程时ECU检测到错误发生；
        wrongBlockSequenceCounter					= 0x73, //错误的数据块序列计数器，ECU在数据块序列计数序列中检测到错误发生；
        requestCorrectlyReceived_ResponsePending	= 0x78, //正确接收请求消息-等待响应 表明ECU正确接收到请求消息，但是将执行的动作未完成且ECU未准备好接收其它请求；
        subFunctionNotSupportedInActiveSession		= 0x7E, //激活会话不支持该子服务，当前会话模式下ECU不支持请求的子服务；
        serviceNotSupportedInActiveSession			= 0x7F, //激活会话不支持该服务，当前会话模式下ECU不支持请求的服务；
        voltageTooHigh								= 0x92, //电压过高，当前电压值超过了编程允许的最大门限值；
        voltageTooLow								= 0x93, //电压过低，当前电压值低于了编程允许的最小门限值
    };

    struct DiagnosticMsg
    {
        enum DiagnosticMsgType type;
        quint16 sourceAddr;
        quint16 targetAddr;
        quint8  code;   // ACKorNACK code
        QByteArray udsData;
    };

    struct VehicleAnnouncement
    {
        QString VIN;
        quint16 logicalAddr;
        QByteArray EID;
        QByteArray GID;
        quint8 Fur;
        quint8 syncSta;
    };

    const int DoIPHeaderLen = 8;

    // 外部传进来数据，不可改动，仅能解析
    doipPacket(QByteArray &arr);
    bool isUnchangeable();

    // 内部创造数据，可以改动，创造
    doipPacket(quint16 payloadType, QByteArray &payload);
    doipPacket();
    ~doipPacket();

    bool creatHeader(quint16 payloadType, quint32 payloadSize);

    QByteArray& Data(void);

    bool isVehicleIdentificationRequest();
    bool VehicleIdentificationRequestAnalyze(QString &VIN, QByteArray &EID);
    bool VehicleIdentificationRequest();
    bool VehicleIdentificationRequest(QString VIN);
    bool VehicleIdentificationRequest(QByteArray EID);
    bool VehicleAnnouncement(QString VIN, quint16 logicalAddr,
                                    QByteArray EID, QByteArray GID,
                                    quint8 Fur, quint8 syncSta);
    bool VehicleAnnouncementAnalyze(struct VehicleAnnouncement &vic);
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
