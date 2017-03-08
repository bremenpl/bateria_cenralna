#include "cbcrc.h"

CBcRc::CBcRc(const quint16 slaveAddr,
             const quint32 pingsMax,
             const QVector<slaveId*>* pv,
             QObject *parent) :
    CBcSlaveDevice(slaveAddr, pingsMax, pv, parent)
{
    m_slaveId.m_slaveType = EDeviceTypes::RelayCtrler;
}

void CBcRc::statusRegBitOperation(bool set, quint32 bit)
{
    if (set)
        m_statusReg |= 1 << bit;
    else
        m_statusReg &= ~(1 << bit);
}
