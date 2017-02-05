#include "cbcrc.h"

CBcRc::CBcRc(const quint16 slaveAddr,
             const quint32 pingsMax,
             const QVector<slaveId*>* pv,
             QObject *parent) :
    CBcSlaveDevice(slaveAddr, pingsMax, pv, parent)
{
    m_slaveId.m_slaveType = EDeviceTypes::RelayCtrler;
}
