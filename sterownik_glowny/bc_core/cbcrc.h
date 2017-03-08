#ifndef CBCRC_H
#define CBCRC_H

#include <QObject>
#include "cbcslavedevice.h"

#define STATUS_REG_RELAY_BIT        0

class CBcRc : public CBcSlaveDevice
{
    Q_OBJECT
public:
    explicit CBcRc(const quint16 slaveAddr,
                   const quint32 pingsMax = 0,
                   const QVector<slaveId*>* pv = 0,
                   QObject *parent = 0);

    void statusRegSet(quint16 val) { m_statusReg = val; }
    void statusRegBitOperation(bool set, quint32 bit);
    quint16 statusReg() { return m_statusReg; }

signals:

public slots:

private:
    quint16     m_statusReg = 0;

};

#endif // CBCRC_H
