#include "cbclcthread.h"

CBcLcThread::CBcLcThread(const QString& port,
                         const quint32 noOfPings,
                         const quint32 noOfDevices,
                         QObject *parent) : CBcSerialThread(port, noOfPings, noOfDevices, parent)
{
    // create the vector of slaves
    for (quint32 i = 0; i < noOfDevices; i++)
        m_slaves.append(new CBcLc(i + 1, noOfPings, this));
}

CBcLcThread::~CBcLcThread()
{

}

void CBcLcThread::responseReady_ReadHoldingRegistersOverride(const quint8 slaveId,
                                                         const quint16 startAddr,
                                                         const QVector<quint16>& registers)
{
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                 "Read holding registers response (LC). SID:%u, SADDR:%u, NOOFREGS:%u",
                                 slaveId, startAddr, registers.size());
}
