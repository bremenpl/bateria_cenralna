#include "ctcpparser.h"

CTcpParser::CTcpParser(QObject *parent) : QObject(parent)
{

}

QVector<slaveId*> CTcpParser::getParentVector(const tcpFrame* const frame,
                                              const int depth,
                                              const int dataLen)
{
    QVector<slaveId*> pv;

    for (int i = 0; i < depth; i++)
    {
        slaveId* slv =  new slaveId;
        slv->m_slaveAddr = frame->data[dataLen + i * 2];
        slv->m_slaveType = (EDeviceTypes)((quint8)frame->data[dataLen + 1 + i * 2]);
        pv.append(slv);
    }

    return pv;
}

void CTcpParser::digForTcpFrames(const QByteArray& data)
{
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "New data rcv from server (%i bytes)", data.size());

    foreach (quint8 byte, data)
    {
        switch (m_tcpRxState)
        {
            case tcpRespState::devType:
            {
                mp_rxTcpFrame = new tcpFrame;
                mp_rxTcpFrame->data.clear();
                mp_rxTcpFrame->dType = (EDeviceTypes)byte;
                m_tcpRxState = tcpRespState::slaveAddr;
                break;
            }

            case tcpRespState::slaveAddr:
            {
                mp_rxTcpFrame->slaveAddr = byte;
                m_tcpRxState = tcpRespState::req;
                break;
            }

            case tcpRespState::req:
            {
                mp_rxTcpFrame->req = (tcpReq)byte;
                m_tcpRxState = tcpRespState::cmd;
                break;
            }

            case tcpRespState::cmd:
            {
                mp_rxTcpFrame->cmd = (tcpCmd)byte;
                m_tcpRxState = tcpRespState::len;
                break;
            }

            case tcpRespState::len:
            {
                mp_rxTcpFrame->len = (int)byte;
                m_tcpRxState = tcpRespState::data;
                break;
            }

            case tcpRespState::data:
            {
                mp_rxTcpFrame->data.append(byte);

                if (mp_rxTcpFrame->data.size() >= mp_rxTcpFrame->len)
                {
                    // frame fully parsed
                    m_rxTcpQueue.enqueue(mp_rxTcpFrame);

                    // set initial state
                    mp_rxTcpFrame = 0;
                    m_tcpRxState = tcpRespState::devType;

                    emit newFramesAvailable(&m_rxTcpQueue);
                }
                break;
            }

            default:
            {
                CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                        << "Unhandled tcpRxState: " << (int)m_tcpRxState;

                // initial state
                m_tcpRxState = tcpRespState::devType;

                if (mp_rxTcpFrame)
                {
                    delete mp_rxTcpFrame;
                    mp_rxTcpFrame = 0;
                }
            }
        }
    }
}
