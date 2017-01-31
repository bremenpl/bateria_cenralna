#ifndef CBCSLAVEDEVICE_H
#define CBCSLAVEDEVICE_H

#include <QObject>

#include "cbctcpserver.h"

class CBcSlaveDevice : public QObject
{
    Q_OBJECT
public:
    explicit CBcSlaveDevice(const quint16 slaveAddr, const quint32 pingsMax, QObject *parent = 0);

    bool managePresence(const bool response);
    bool presence() { return m_presence; }

signals:
    void sendDataAck(const tcpFrame& frame);

public slots:

private:

protected:
    // members
    quint16     m_slaveAddr;
    quint32     m_pingsMax;
    quint32     m_pings;
    bool        m_presence;
    bool        m_presenceOld;
};

#endif // CBCSLAVEDEVICE_H
