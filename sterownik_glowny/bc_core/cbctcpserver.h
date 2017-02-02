#ifndef CBCTCPSERVER_H
#define CBCTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QList>

#include "cbclogger.h"
#include "cbcclientthread.h"

enum class devType
{
    None    = 0,
    Lc      = 1,
    Bat     = 2,
    Rc      = 3,
};

enum class tcpCmd
{
    presenceChanged = 1,
};

enum class tcpReq
{
    set = 0,
    get = 1,
};

struct tcpFrame
{
    devType dType;      /*!< LC or BAT */
    quint8 slaveAddr;   /*!< Modbus slave address */
    tcpReq req;         /*!< Set value or get value */
    tcpCmd cmd;         /*!< for example presence changed, turn relay on/off */
    QByteArray data;    /*!< data part */
};

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
    void on_clientDisconnected(QTcpSocket* socket);
    void on_modbusStatusChanged(quint16 status);

    void on_sendDataAck(const tcpFrame& frame);

protected:
    void incomingConnection(qintptr socketDescriptor);

private:
    QList<QTcpSocket*>    m_connectedSockets;
};

#endif // CBCTCPSERVER_H
