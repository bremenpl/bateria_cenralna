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
    void precenceSet(const bool val);
    QVector<CBcSlaveDevice*> subSlaves() { return m_subSlaves; }

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
    devType     m_devType;

    QVector<CBcSlaveDevice*> m_subSlaves;      /*!< subslaves of a slave, ie. rc's of an lc */
};

#endif // CBCSLAVEDEVICE_H
