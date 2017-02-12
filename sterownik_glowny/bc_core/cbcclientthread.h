#ifndef CBCCLIENTTHREAD_H
#define CBCCLIENTTHREAD_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>
#include <QQueue>

#include "cbclogger.h"
#include "types.h"
#include "ctcpparser.h"
#include "cbcslavedevice.h"

class CBcClientThread : public QThread
{
    Q_OBJECT
public:
    explicit CBcClientThread(qintptr ID, QObject *parent = 0);

    void run();

signals:
    void error(QTcpSocket::SocketError socketerror);
    void clientConnected(QTcpSocket* socket);
    void clientDisconnected(QTcpSocket* socket);
    void sendData2ModbusSlave(const tcpReq req,
                              const tcpCmd cmd,
                              const QVector<slaveId*>& pv,
                              const QByteArray& data);

public slots:
    void readyRead();
    void disconnected();
    void on_sendData2Socket(const QByteArray& data);
    void on_newFramesAvailable(QQueue<tcpFrame*>* framesQueue);

private:
    QTcpSocket  *mp_socket;
    qintptr     m_socketDescriptor;

    CTcpParser  m_tcpParser;
};

#endif // CBCCLIENTTHREAD_H
