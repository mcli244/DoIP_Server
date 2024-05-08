#include "uds.h"
#include <QDebug>

udsMsg::udsMsg()
{

}

udsMsg::udsMsg(QByteArray &uds)
{
    _udsPacket = &uds;
}

bool udsMsg::isUdsResponse(struct udsResponse &udsResp)
{
    // TODO
    if(!_udsPacket->size()) return false;

    if(_udsPacket->at(0) == 0x7f)
    {
        // 7F + 服务 + NRC（negative response Code）
        udsResp.isPositiveResponse = false;
        udsResp.SID     = _udsPacket->at(1);
        udsResp.NRC     = _udsPacket->at(2);
        return true;
    }
    else
    {
        // SID+0x40 + 其他
        udsResp.isPositiveResponse = true;
        udsResp.SID     = _udsPacket->at(0) - 0x40;
        udsResp.Dat     = _udsPacket->mid(1);
        return true;
    }

    return true;
}

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

char _ByteToChar(quint8 byte)
{
    if(byte>=0 && byte<=9)
    {
        return byte + 0x30;
    }
    return ' ';
}

bool _BytesToDTC(QByteArray &dtc, QString &str)
{
    if(dtc.size() < 4)
    {
        qDebug() << "size:" << dtc.size();
        return false;
    }

    str.resize(5);

    switch(dtc.at(0)>>6 & 0x03)
    {
        case 0: str = 'P';   break;
        case 1: str = 'C';   break;
        case 2: str = 'B';   break;
        case 3: str = 'U';   break;
        default: return false;
    }

    str += _ByteToChar(dtc.at(0)>>4 & 0x03);
    str += _ByteToChar(dtc.at(0) & 0x0F);

    str += _ByteToChar(dtc.at(1)>>4 & 0x0F);
    str += _ByteToChar(dtc.at(1) & 0x0F);

    qDebug() << "DTC: " << dtc.toHex(' ') << "错误码:" << str;

    return true;
}

bool _BytesToDTCDescription(QByteArray &dtc, QString &str)
{
    // TODO
    str = "发动机温度过低";

    return true;
}

bool _BytesToDTCState(QByteArray &dtc, QString &str)
{
    // TODO
    str = "当前";

    return true;
}

bool udsMsg::isPositiveResponse()
{
    if(_udsPacket->at(0) == 0x7f)
        return false;
    return true;
}

bool udsMsg::DTCAnalyze(struct udsDtc &udsDTC)
{
    if(!_udsPacket->size())
    {
        qDebug() << "Packet is null";
        return false;
    }

    if(!isPositiveResponse())
    {
        qDebug() << QString::asprintf("negative response sid:0x%x nrc:%d\n",
                                      _udsPacket->at(1), _udsPacket->at(2));
        return false;
    }

    udsDTC.SID     = _udsPacket->at(0) - 0x40;
    udsDTC.SubID   = _udsPacket->at(1);
    udsDTC.mask    = _udsPacket->at(2);

    if(udsDTC.SID != 0x19)
    {
        qDebug()<<"not is dtc response";
        return false;
    }

    qint16 index = 0;

    switch(udsDTC.SubID)
    {
    case 0x02: index = 3; break;
    case 0x03: index = 3; break;
    case 0x04: index = 3; break;
    case 0x0a: index = 2; break;
    default:   index = 3; break;
    }

    for(; index<_udsPacket->size() - 3; index+=4)
    {
        QByteArray dtc = _udsPacket->mid(index, 4);

        struct dtcInfo dtcInf;
        dtcInf.DTCHighByte      = dtc.at(0);
        dtcInf.DTCMiddleByte    = dtc.at(1);
        dtcInf.DTCLowByte       = dtc.at(2);
        dtcInf.StatusOfDTC      = dtc.at(3);

        _BytesToDTC(dtc, dtcInf.eCodeStr);
        _BytesToDTCDescription(dtc, dtcInf.description);
        _BytesToDTCState(dtc, dtcInf.state);
        udsDTC.dtcList.append(dtcInf);
    }

    return true;
}
