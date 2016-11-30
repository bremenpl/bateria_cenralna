#ifndef CBCSERIALTHREAD_H
#define CBCSERIALTHREAD_H

#include <QThread>
#include <QString>
#include <QModbusClient>
#include <QModbusRtuSerialMaster>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QVariant>

#include "cbclogger.h"

class CBcSerialThread : public QThread
{
    Q_OBJECT
public:
    explicit CBcSerialThread(const QString& port, QObject *parent = 0);
    ~CBcSerialThread();

    void run(); // inherited

private:
    // members
    QModbusClient *mp_modbusDevice;

};

#endif // CBCSERIALTHREAD_H
