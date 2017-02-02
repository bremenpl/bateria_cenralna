#include "cbcrc.h"

CBcRc::CBcRc(const quint16 slaveAddr, const quint32 pingsMax, QObject *parent) :
    CBcSlaveDevice(slaveAddr, pingsMax, parent)
{
    m_devType = devType::Rc;
}
