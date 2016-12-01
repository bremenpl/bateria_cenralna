#ifndef CBCSERIALTHREAD_H
#define CBCSERIALTHREAD_H

#include <QThread>
#include <QString>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QTimer>

#include "cbclogger.h"
#include "csmrm.h"

class CBcSerialThread : public QThread
{
    Q_OBJECT
public:
    explicit CBcSerialThread(const QString& port, QObject *parent = 0);
    ~CBcSerialThread();

    void run(); // inherited

public slots:
    void on_responseReady_ReadHoldingRegisters(const quint8 slaveId, const QVector<quint16>& registers);

signals:

private:
    // members
    Csmrm*  mp_modbusMaster;
    QTimer*  mp_pollTimer;

private slots:
    void on_pollTimeout();

};

#endif // CBCSERIALTHREAD_H
