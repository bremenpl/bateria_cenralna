#ifndef TYPES_H
#define TYPES_H

#include <QByteArray>


enum class EBtnTypes
{
    Devices,
    Tests,
    Logs,
    Settings,
    Batteries,
    LineControllers,
    RelayControllers,
    PowerAdapter,
    Chargers,
};

enum class EDeviceTypes
{
    Dummy,
    LineCtrler,
    RelayCtrler,
    Battery,
    Charger,
};

enum class tcpCmd
{
    dummy           = 0,
    presenceChanged = 1,
};

enum class tcpReq
{
    set = 0,
    get = 1,
};

struct tcpFrame
{
    EDeviceTypes dType; /*!< LC or BAT */
    quint8 slaveAddr;   /*!< Modbus slave address */
    tcpReq req;         /*!< Set value or get value */
    tcpCmd cmd;         /*!< for example presence changed, turn relay on/off */
    int len;            /*!< Expected data length */
    QByteArray data;    /*!< data part */
};

enum class tcpRespState
{
    devType             = 0,    /*!< Device type */
    slaveAddr           = 1,    /*!< Modbus slave ID */
    req                 = 2,    /*!< Set or Get */
    cmd                 = 3,    /*!< Command */
    len                 = 4,    /*!< Data length */
    data                = 5,    /*!< Data part till the end */
};

struct slaveId
{
    quint8           m_slaveAddr;
    EDeviceTypes     m_slaveType;
};
















#endif // TYPES_H
