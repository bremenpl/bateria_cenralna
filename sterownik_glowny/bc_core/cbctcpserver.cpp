#include "cbctcpserver.h"

CBcTcpServer::CBcTcpServer(QObject *parent) : QTcpServer(parent)
{
    qRegisterMetaType<EDeviceTypes>("EDeviceTypes");
    qRegisterMetaType<tcpCmd>("tcpCmd");
    qRegisterMetaType<tcpFrame>("tcpFrame");
}

void CBcTcpServer::startServer()
{
    int port = 12345;

    if(!listen(QHostAddress::Any, port))
        CBcLogger::instance()->print(MLL::ELogLevel::LCritical, "Could not start TCP server");
    else
        CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Tcp server started, listening on port %u", port);
}

// This function is called by QTcpServer when a new connection is available.
void CBcTcpServer::incomingConnection(qintptr socketDescriptor)
{
    // We have a new connection
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Client 0x%X connecting...", socketDescriptor);

    // Every new connection will be run in a newly created thread
    CBcClientThread* thread = new CBcClientThread(socketDescriptor);
    thread->moveToThread(thread);

    connect(thread, SIGNAL(clientDisconnected(QTcpSocket*)),
            this, SLOT(on_clientDisconnected(QTcpSocket*)), Qt::QueuedConnection);

    connect(thread, SIGNAL(clientConnected(QTcpSocket*)),
            this, SLOT(on_clientConnected(QTcpSocket*)), Qt::QueuedConnection);

    // data sending connection (because socket is in different thread)
    connect(this, SIGNAL(sendData2Socket(const QByteArray&)),
           thread, SLOT(on_sendData2Socket(const QByteArray&)), Qt::QueuedConnection);

    thread->start();
}

void CBcTcpServer::on_clientConnected(QTcpSocket* socket)
{
    // append new socket to the list
    m_connectedSockets.append(socket);

    // inform the slave devices
    emit newClientConnected();
}

void CBcTcpServer::on_clientDisconnected(QTcpSocket* socket)
{
    // find the socket and remove it
    for (int i = 0; i < m_connectedSockets.length(); i++)
    {
        if (m_connectedSockets[i] == socket)
        {
            m_connectedSockets.removeAt(i);
            break;
        }
    }
}

void CBcTcpServer::on_sendDataAck(const tcpFrame& frame)
{
    // send the frame to the clients
    if (!m_connectedSockets.length())
        return;

    QByteArray rb;
    rb.append((quint8)frame.dType);     // device type
    rb.append(frame.slaveAddr);         // slave address
    rb.append((quint8)frame.req);       // request type
    rb.append((quint8)frame.cmd);       // command
    rb.append((quint8)frame.len);       // data len
    rb.append(frame.data);              // data

    emit sendData2Socket(rb);
}








