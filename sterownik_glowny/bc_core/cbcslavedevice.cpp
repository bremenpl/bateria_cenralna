#include "cbcslavedevice.h"
#include "cbclogger.h"

CBcSlaveDevice::CBcSlaveDevice(const quint16 slaveAddr,
                               const quint32 pingsMax,
                               const QVector<slaveId*>* pv,
                               QObject* parent) : QObject(parent)
{
    qRegisterMetaType<QVector<CBcSlaveDevice*>>("QVector<CBcSlaveDevice*>");

    m_pings = 0;
    m_presence = false;
    m_presenceOld = false;
    m_pingsMax = pingsMax;
    m_slaveId.m_slaveAddr = slaveAddr;
    memset(m_uniqId, 0, UNIQ_ID_REGS * 2);

    // initialy no device
    m_slaveId.m_slaveType = EDeviceTypes::Dummy;

    // cast the parent
    if (parent)
    {
        CBcTcpServer* server = dynamic_cast<CBcTcpServer*>(parent);

        // connect
        connect(this, SIGNAL(sendDataAck(const tcpFrame&)),
                server, SLOT(on_sendDataAck(const tcpFrame&)), Qt::QueuedConnection);

        connect(server, SIGNAL(newClientConnected()),
                this, SLOT(on_newClientConnected()), Qt::QueuedConnection);
    }

    // append parent vector
    if (pv)
    {
        foreach (auto item, *pv)
            m_pv.append(item);
    }

    // append myselfe
    m_pv.append(&m_slaveId);
}

/*!
 * \brief CBcSlaveDevice::managePresence: Manages presence for the slave device.
 * \param response: true- gain presence, false- loose it
 * \return true if presence changed
 */
bool CBcSlaveDevice::managePresence(const bool response)
{
    if (response)
    {
        // increment pings if possible
        if (m_pings >= (m_pingsMax - 1))
        {
            if (!m_presence) // present
            {
                m_pings++;
                precenceSet(response);

                CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Slave 0x%02X present", m_slaveId.m_slaveAddr);
            }
        }
        else // not present yet
        {
            m_pings++;
            CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                         "Slave 0x%02X gaining presence (%u)", m_slaveId.m_slaveAddr, m_pings);
        }
    }
    else
    {
        if (m_pings <= 1)
        {
            if (m_presence) // just lost presence
            {
                m_pings--;
                precenceSet(response);

                // reset child presences
                clearChildPresence();

                CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Slave 0x%02X absent", m_slaveId.m_slaveAddr);
            }
        }
        else // loosing it
        {
            m_pings--;
            CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                         "Slave 0x%02X loosing presence (%u)", m_slaveId.m_slaveAddr, m_pings);
        }
    }

    // check if presence changed
    bool presenceChanged = false;
    if (m_presence != m_presenceOld)
        presenceChanged = true;

    // update old presence
    m_presenceOld = m_presence;

    return presenceChanged;
}

void CBcSlaveDevice::precenceSet(const bool val)
{
    if (val != m_presence)
    {
        m_presence = val;
        presenceSend();
    }
}

void CBcSlaveDevice::presenceSend()
{
    tcpFrame frame;
    frame.dType = m_slaveId.m_slaveType;
    frame.slaveAddr = m_slaveId.m_slaveAddr;
    frame.req = tcpReq::get;
    frame.cmd = tcpCmd::presenceChanged;
    frame.data.append(quint8(m_presence));

    // append parent vector
    foreach (slaveId* item, m_pv)
    {
        frame.data.append(item->m_slaveAddr);
        frame.data.append((quint8)item->m_slaveType);
    }

    frame.len = frame.data.size();
    emit sendDataAck(frame);
}

void CBcSlaveDevice::sendGetCmdToClients(const tcpCmd cmd, const QVector<quint16>& regs)
{
    tcpFrame frame;
    frame.dType = m_slaveId.m_slaveType;
    frame.slaveAddr = m_slaveId.m_slaveAddr;
    frame.req = tcpReq::get;
    frame.cmd = cmd;

    // data
    foreach (auto reg, regs)
    {
        frame.data.append((quint8)reg);
        frame.data.append((quint8)(reg >> 8));
    }

    // append parent vector
    foreach (slaveId* item, m_pv)
    {
        frame.data.append(item->m_slaveAddr);
        frame.data.append((quint8)item->m_slaveType);
    }

    frame.len = frame.data.size();
    emit sendDataAck(frame);
}

/*!
 * \brief CBcSlaveDevice::clearChildPresence: A slave shall clear its presense and also
 * clear the presence of its each subslave. That each subslave will also clear its presence
 * and the precense of its subslaves and so on.
 */
void CBcSlaveDevice::clearChildPresence()
{
    m_presence = false;

    // no need to send absence of a child, client knows that when father is absent children are as well
    // presenceSend();

    if (m_subSlaves.size())
        foreach (CBcSlaveDevice* slave, m_subSlaves)
            slave->clearChildPresence();
}

void CBcSlaveDevice::on_newClientConnected()
{
    // decide either one should broadcast its presence
    if (m_presence)
        presenceSend();
}













