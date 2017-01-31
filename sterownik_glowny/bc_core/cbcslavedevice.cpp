#include "cbcslavedevice.h"
#include "cbclogger.h"

CBcSlaveDevice::CBcSlaveDevice(const quint16 slaveAddr, const quint32 pingsMax, QObject *parent) : QObject(parent)
{
    m_pings = 0;
    m_presence = false;
    m_presenceOld = false;
    m_pingsMax = pingsMax;
    m_slaveAddr = slaveAddr;

    // cast the parent
    CBcTcpServer* server = dynamic_cast<CBcTcpServer*>(parent);

    // connect
      connect(this, SIGNAL(sendDataAck(const tcpFrame&)),
              server, SLOT(on_sendDataAck(const tcpFrame&)), Qt::UniqueConnection);
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
                m_presence = true;
                CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Slave 0x%02X present", m_slaveAddr);

                tcpFrame frame;
                frame.dType = devType::Lc;
                frame.slaveAddr = m_slaveAddr;
                frame.req = tcpReq::get;
                frame.cmd = tcpCmd::presenceChanged;
                frame.data.append(1);

                emit sendDataAck(frame);
            }
        }
        else // not present yet
        {
            m_pings++;
            CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                         "Slave 0x%02X gaining presence (%u)", m_slaveAddr, m_pings);
        }
    }
    else
    {
        if (m_pings <= 1)
        {
            if (m_presence) // just lost presence
            {
                m_pings--;
                m_presence = false;
                CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Slave 0x%02X absent", m_slaveAddr);

                tcpFrame frame;
                frame.dType = devType::Lc;
                frame.slaveAddr = m_slaveAddr;
                frame.req = tcpReq::get;
                frame.cmd = tcpCmd::presenceChanged;
                quint8 temp = 0;
                frame.data.append(temp);

                emit sendDataAck(frame);
            }
        }
        else // loosing it
        {
            m_pings--;
            CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                         "Slave 0x%02X loosing presence (%u)", m_slaveAddr, m_pings);
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
