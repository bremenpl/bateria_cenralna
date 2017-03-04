#include "cbclcthread.h"

CBcLcThread::CBcLcThread(const QString& port,
                         const quint32 noOfPings,
                         const quint32 noOfDevices,
                         const quint32 noOfRcPerLc,
                         QObject *parent) : CBcSerialThread(port, noOfPings, noOfDevices, parent)
{
    // create the vector of slaves
    for (quint32 i = 0; i < noOfDevices; i++)
    {
        // create slave
        m_slaves.append(new CBcLc(i + 1, noOfPings, 0, parent));

        // crate subslaves (rcs)
        for (quint32 k = 0; k < noOfRcPerLc; k++)
        {
            m_slaves[i]->subSlaves().append(
                        new CBcRc(k + 1, noOfPings, m_slaves[i]->parentVector(), parent));
        }
    }

}

CBcLcThread::~CBcLcThread()
{

}

void CBcLcThread::responseReady_ReadHoldingRegistersOverride(const quint8 slaveId,
                                                         const quint16 startAddr,
                                                         const QVector<quint16>& registers)
{
    if (EAddrCodes::Ping == (EAddrCodes)startAddr)
    {
        for (int regs = 0; regs < registers.length(); regs++)
        {
            for (int bits = 0; bits < 16; bits++)
            {
                int rcNr = regs * 16 + bits;

                if (m_slaves[slaveId - 1]->subSlaves().size() > rcNr)
                {
                    bool presence = false;
                    if (registers[regs] & (1 << bits))
                        presence = true;

                    m_slaves[slaveId - 1]->subSlaves()[rcNr]->precenceSet(presence);
                }

            }
        }
    }
    else if (startAddr > 0xFF) // delegate
    {
        uint16_t lcAddr = startAddr / 0xFF;
        uint16_t rcAddr = startAddr & 0xFF;

        m_slaves[lcAddr - 1]->subSlaves()[rcAddr - 1]->sendGetCmdToClients(tcpCmd::takeUniqId, registers);
    }
}


























