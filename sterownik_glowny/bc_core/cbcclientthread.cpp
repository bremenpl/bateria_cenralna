#include "cbcclientthread.h"

CBcClientThread::CBcClientThread(qintptr ID, QObject *parent) : QThread(parent)
{
    m_socketDescriptor = ID;
}

void CBcClientThread::run()
{
    // thread starts here
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Thread for socket 0x%X started", m_socketDescriptor);

    mp_socket = new QTcpSocket();

    // set the ID
    if(!mp_socket->setSocketDescriptor(m_socketDescriptor))
    {
        // something's wrong, we just emit a signal
        emit error(mp_socket->error());
        return;
    }

    // connect socket and signal
    // note - Qt::DirectConnection is used because it's multithreaded
    //        This makes the slot to be invoked immediately, when the signal is emitted.

    connect(mp_socket, SIGNAL(readyRead()), this, SLOT(readyRead()), Qt::DirectConnection);
    connect(mp_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));

    // new way of signal/slot connecting
    connect(&m_tcpParser, &CTcpParser::newFramesAvailable,
            this, &CBcClientThread::on_newFramesAvailable, Qt::QueuedConnection);

    // We'll have multiple clients, we want to know which is which
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Socket 0x%X is connected", m_socketDescriptor);

    // emit connection signal
    emit clientConnected(mp_socket);

    // make this thread a loop,
    // thread will stay alive so that signal/slot to function properly
    // not dropped out in the middle when thread dies

    exec();
}

void CBcClientThread::readyRead()
{
    // get the information
    QByteArray data = mp_socket->readAll();

    CBcLogger::instance()->print(MLL::ELogLevel::LDebug,
        "Socket 0x%X received %i bytes of data", m_socketDescriptor, data.size());

    m_tcpParser.digForTcpFrames(data);
}

void CBcClientThread::disconnected()
{
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Socket 0x%X disconnected", m_socketDescriptor);

    emit clientDisconnected(mp_socket);
    mp_socket->deleteLater();
    exit(0);
}

void CBcClientThread::on_sendData2Socket(const QByteArray& data)
{
    quint32 lenSent = mp_socket->write(data, data.length());
    if (lenSent)
        CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                     "%u bytes sent to socket 0x%X", lenSent, m_socketDescriptor);
    else
        CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                                     "Failed to send data to socket 0x%X", m_socketDescriptor);
}

void CBcClientThread::on_newFramesAvailable(QQueue<tcpFrame*>* framesQueue)
{
    Q_ASSERT(framesQueue);

    if (framesQueue->size())
    {
        tcpFrame* frame = framesQueue->dequeue();
        QVector<slaveId*> pv; // parent vector
        QByteArray data;
        int depthLevel = 0, dataLen = -1;

        // switch needed only to insert command specific data len
        switch (frame->cmd)
        {
            case tcpCmd::takeUniqId: dataLen = 0;   break; // only get available for bc_core

            default:
            {
                CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                        << "Unhandled tcp Rx cmd: " << (int)frame->cmd;
            }
        }

        if (dataLen >= 0)
        {
            depthLevel = (frame->len - dataLen) / 2;
            pv.append(CTcpParser::getParentVector(frame, depthLevel, dataLen));

            // send the data further
            emit sendData2ModbusSlave(frame->req, frame->cmd, pv, data);
        }

        // finally delete this frame
        delete frame;
        pv.clear();
    }
}















