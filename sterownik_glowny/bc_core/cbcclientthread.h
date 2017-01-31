#ifndef CBCCLIENTTHREAD_H
#define CBCCLIENTTHREAD_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>

#include "cbclogger.h"

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

public slots:
    void readyRead();
    void disconnected();
    void on_sendData2Socket(const QByteArray& data);

private:
    QTcpSocket  *mp_socket;
    qintptr     m_socketDescriptor;
};

#endif // CBCCLIENTTHREAD_H
