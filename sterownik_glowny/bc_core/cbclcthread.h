#ifndef CBCLCTHREAD_H
#define CBCLCTHREAD_H

#include <QObject>

#include "cbcserialthread.h"
#include "cbclc.h"

class CBcLcThread : public CBcSerialThread
{
    Q_OBJECT
public:
    explicit CBcLcThread(const QString& port,
                         const quint32 noOfPings,
                         const quint32 noOfDevices,
                         QObject *parent = 0);
    ~CBcLcThread();

    virtual void responseReady_ReadHoldingRegistersOverride(const quint8 slaveId,
                                                            const quint16 startAddr,
                                                            const QVector<quint16>& registers);

signals:

public slots:

private:
    // members


};

#endif // CBCLCTHREAD_H
