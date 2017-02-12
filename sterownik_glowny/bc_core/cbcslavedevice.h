#ifndef CBCSLAVEDEVICE_H
#define CBCSLAVEDEVICE_H

#include <QObject>
#include "cbctcpserver.h"
#include "types.h"

#define UNIQ_ID_REGS    6

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
    slaveId& slave_id() { return m_slaveId; }
    const QVector<slaveId*>* parentVector() { return &m_pv; }
    void clearChildPresence();
    void setUniqId(const quint16* uniqId) { memcpy(m_uniqId, uniqId, UNIQ_ID_REGS * 2); }
    quint16* uniqId() { return m_uniqId; }

    void sendGetCmdToClients(const tcpCmd cmd, const QVector<quint16>& regs);

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
    quint16     m_uniqId[UNIQ_ID_REGS];

    QVector<CBcSlaveDevice*> m_subSlaves;       /*!< subslaves of a slave, ie. rc's of an lc */
    QVector<slaveId*> m_pv;                     /*!< parent vector */

};

#endif // CBCSLAVEDEVICE_H
