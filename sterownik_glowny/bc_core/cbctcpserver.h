#ifndef CBCTCPSERVER_H
#define CBCTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QList>

#include "cbclogger.h"
#include "cbcclientthread.h"

class CBcTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit CBcTcpServer(QObject *parent = 0);

    void startServer();

signals:
    void sendData2Socket(const QByteArray& data);

public slots:
    void on_clientConnected(QTcpSocket* socket);
    void on_modbusStatusChanged(quint16 status);

protected:
    void incomingConnection(qintptr socketDescriptor);

private:
    QList<QTcpSocket*>    m_connectedSockets;
};

#endif // CBCTCPSERVER_H
