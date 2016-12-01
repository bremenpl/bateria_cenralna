#include "cbctcpserver.h"

CBcTcpServer::CBcTcpServer(QObject *parent) : QTcpServer(parent)
{

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
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Client 0x0X connecting...", socketDescriptor);

    // Every new connection will be run in a newly created thread
    CBcClientThread* thread = new CBcClientThread(socketDescriptor);
    thread->moveToThread(thread);

    // connect signal/slot
    // once a thread is not needed, it will be beleted later
    //connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    // after connection is established, get notified
    connect(thread, SIGNAL(clientConnected(QTcpSocket*)),
                           this, SLOT(on_clientConnected(QTcpSocket*)), Qt::DirectConnection);
    // data sending connection (because socket is in different thread)
    connect(this, SIGNAL(sendData2Socket(const QByteArray&)),
           thread, SLOT(on_sendData2Socket(const QByteArray&)), Qt::UniqueConnection);

    thread->start();
}

void CBcTcpServer::on_clientConnected(QTcpSocket* socket)
{
    // append new socket to the list
    m_connectedSockets.append(socket);
}

void CBcTcpServer::on_modbusStatusChanged(quint16 status)
{
    if (m_connectedSockets.length())
    {
        CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Status changed %u", status);
        QByteArray rb;
        rb.append((quint8)status);
        rb.append((quint8)(status >> 8));

        // send broadcast data, maybe change this later
        sendData2Socket(rb);
    }
}






