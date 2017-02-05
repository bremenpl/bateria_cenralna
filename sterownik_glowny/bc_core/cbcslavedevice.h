#ifndef CBCSLAVEDEVICE_H
#define CBCSLAVEDEVICE_H

#include <QObject>
#include "cbctcpserver.h"
#include "types.h"

class CBcSlaveDevice : public QObject
{
    Q_OBJECT
public:
    explicit CBcSlaveDevice(const quint16 slaveAddr,
                            const quint32 pingsMax,
                            const QVector<slaveId*>* pv = 0,
                            QObject* parent = 0);

    bool managePresence(const bool response);
    bool presence() { return m_presence; }
    void precenceSet(const bool val);
    void presenceSend();
    QVector<CBcSlaveDevice*>& subSlaves() { return m_subSlaves; }
    const QVector<slaveId*>* parentVector() { return &m_pv; }
    void clearChildPresence();

signals:
    void sendDataAck(const tcpFrame& frame);

public slots:
    void on_newClientConnected();

private:

protected:
    // members
    slaveId     m_slaveId;
    quint32     m_pingsMax;
    quint32     m_pings;
    bool        m_presence;
    bool        m_presenceOld;

    QVector<CBcSlaveDevice*> m_subSlaves;       /*!< subslaves of a slave, ie. rc's of an lc */
    QVector<slaveId*> m_pv;                     /*!< parent vector */

};

#endif // CBCSLAVEDEVICE_H
