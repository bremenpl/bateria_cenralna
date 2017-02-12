#ifndef CTCPPARSER_H
#define CTCPPARSER_H

#include <QObject>
#include <QQueue>
#include <QByteArray>
#include <QVector>

#include "types.h"
#include "cbclogger.h"

class CTcpParser : public QObject
{
    Q_OBJECT
public:
    explicit CTcpParser(QObject *parent = 0);

    void digForTcpFrames(const QByteArray& data);
    static QVector<slaveId*> getParentVector(const tcpFrame* const frame,
                                             const int depth = 0,
                                             const int dataLen = 0);

signals:
    void newFramesAvailable(QQueue<tcpFrame*>* framesQueue);

public slots:

private:
    tcpRespState        m_tcpRxState = tcpRespState::devType;
    tcpFrame*           mp_rxTcpFrame = 0;
    QQueue<tcpFrame*>   m_rxTcpQueue;
};

#endif // CTCPPARSER_H
