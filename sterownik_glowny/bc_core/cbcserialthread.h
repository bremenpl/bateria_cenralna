#ifndef CBCSERIALTHREAD_H
#define CBCSERIALTHREAD_H

#include <QThread>
#include <QString>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QTimer>
#include <QVector>

#include "cbclogger.h"
#include "csmrm.h"
#include "cbcslavedevice.h"

#define MAX_PINGS           30

#define NO_OF_PING_REGS     2 // a const for now
#define NO_OF_UNIQDID_REG   6
#define NO_OF_STATUS_REGS   1

class CBcSerialThread : public QThread
{
    Q_OBJECT
public:
    enum class EAddrCodesLC
    {
        Ping            = 0,
        UniqId          = 2,
    };

    enum class EAddrCodesRC
    {
        Status          = 0,
        UniqId          = 2,
    };

    enum class ECoilAddrCodesRc
    {
        Relay           = 8,
    };

    explicit CBcSerialThread(const QString& port,
                             const quint32 noOfPings,
                             const quint32 noOfDevices,
                             QObject *parent = 0);
    ~CBcSerialThread();

    void run(); // inherited
    virtual void responseReady_ReadHoldingRegistersOverride(const quint8 slaveId,
                                                            const quint16 startAddr,
                                                            const QVector<quint16>& registers);

    virtual void responseReady_WriteSingleCoil(const quint8 slaveId,
                                               const quint16 addr,
                                               const bool val);
    bool reOpenPort();


public slots:
    void on_setPingParameters(const quint32 noOfPings, const quint32 noOfDevices);

    void on_responseReady_ReadHoldingRegisters(const quint8 slaveId,
                                               const quint16 startAddr,
                                               const QVector<quint16>& registers);

    void on_responseReady_WriteSingleCoil(const quint8 slaveId,
                                          const quint16 addr,
                                          const bool val);

    void on_responseReady_ReadCoils(const quint8 slaveId,
                                    const quint16 startAddr,
                                    const QVector<bool>& coils);

    void on_sendData2ModbusSlave(const tcpReq req,
                                 const tcpCmd cmd,
                                 const QVector<slaveId*>& pv,
                                 const QByteArray& data);

private slots:
    void on_pollTimeout();
    void on_respTimeout();

signals:

private:
    // members
    QTimer*         mp_pollTimer;
    QTimer*         mp_respToutTimer;
    quint32         m_noOfDev2Scan;         /*!< amount of devices to scan from address 1 */
    quint32         m_pingsForPresence;     /*!< amount of pings needed for presence status */
    quint32         m_curScanDev;           /*!< currently scanned device */
    quint32         m_rcOffset;
    quint32         m_curPing = 0;

protected:
    Csmrm*          mp_modbusMaster;
    QVector<CBcSlaveDevice*> m_slaves;      /*!< "list" of line controllers slave devices */

};

#endif // CBCSERIALTHREAD_H
