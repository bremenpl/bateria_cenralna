#ifndef CBCTCPSERVER_H
#define CBCTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QList>

#include "cbclogger.h"
#include "cbcclientthread.h"
#include "types.h"

class CBcTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit CBcTcpServer(QObject *parent = 0);

    void startServer();

signals:
    void sendData2Socket(const QByteArray& data);
    void newClientConnected();
    void sendData2ModbusSlave(const tcpReq req,
                              const tcpCmd cmd,
                              const QVector<slaveId*>& pv,
                              const QByteArray& data);

public slots:
    void on_clientConnected(QTcpSocket* socket);
    void on_clientDisconnected(QTcpSocket* socket);
    void on_sendData2ModbusSlave(const tcpReq req,
                                 const tcpCmd cmd,
                                 const QVector<slaveId*>& pv,
                                 const QByteArray& data);

    void on_sendDataAck(const tcpFrame& frame);

protected:
    void incomingConnection(qintptr socketDescriptor);

private:
    QList<QTcpSocket*>    m_connectedSockets;
};

#endif // CBCTCPSERVER_H
