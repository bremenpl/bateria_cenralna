#include "cbclc.h"

CBcLc::CBcLc(const quint16 slaveAddr, const quint32 pingsMax, QObject *parent) :
    CBcSlaveDevice(slaveAddr, pingsMax, parent)
{

}
