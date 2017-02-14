#ifndef CSMRM_H
#define CSMRM_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QVector>
#include <QByteArray>
#include <QQueue>

#define MAX_QUAN_OF_REGS            126
#define MAX_QUAN_OF_COILS           2000

class Csmrm : public QSerialPort
{
    Q_OBJECT
public:
    enum class EFuncCodes
    {
        ReadDiscreteInputs			= 02,
        ReadCoils 					= 01,
        WriteSingleCoil             = 05,
        WriteMultipleCoils			= 15,
        ReadInputRegister 			= 04,
        ReadHoldingRegisters		= 03,
        WriteSingleRegister         = 06,
        WriteMultipleRegisters		= 16,
        ReadWriteMultipleRegisters	= 23,
        MaskWriteRegister			= 22,
    };

    enum class EExceptionCodes
    {
        NoError                     = 0, // used only for validation, not to be sent
        IllegalFunction             = 0x01,
        IllegalDataAddr             = 0x02,
        IllegalDataValue            = 0x03,
        SlaveDeviceFailure          = 0x04,
        Acknowledge                 = 0x05,
        SlaveDeviceBusy             = 0x06,
        MemoryParityError           = 0x08,
    };

    enum class EResponseState
    {
        SlaveAddr                   = 0,
        FuncCode                    = 1,
        Data                        = 2,
        Crc                         = 3,
    };

    struct SModBusFrame
    {
        quint8      slaveAddr;
        EFuncCodes  functionCode;
        QByteArray  data;
        quint16     crc;
    };

    explicit Csmrm(QObject *parent = 0);
    ~Csmrm();

    void sendRequest_ReadHoldingRegisters(const quint8 slaveId, const quint16 startAddr, const quint16 nrOfRegs);
    void sendRequest_ReadCoils(const quint8 slaveId, const quint16 startAddr, const quint16 nrOfCoils);

    const SModBusFrame& txFrame(){ return m_txFrame; }
    QQueue<SModBusFrame>* txFrameQueue() { return &m_txFramesQueue; }

public slots:
    void on_newTxFrameEnqueued();

signals:
    void responseReady_ReadHoldingRegisters(const quint8 slaveId,
                                            const quint16 startAddr,
                                            const QVector<quint16>& registers);
    void responseReady_ReadCoils(const quint8 slaveId,
                                 const quint16 startAddr,
                                 const QVector<bool>& coils);
    void responseReady_ExceptionCode(const quint8 slaveId, EExceptionCodes code);
    void newTxFrameEnqueued();

private:
    quint16 calculateCRC(const char *data, qint32 len);
    void addCrc2Frame(SModBusFrame& frame);
    qint32 sendFrame(const SModBusFrame& frame);
    void enqueueFrame(const SModBusFrame& frame);
    const QString convertArray2HexString(const QByteArray& array);
    qint32 digForResponse(const QByteArray& buf);
    void setResponseDefaultState();
    void parseResponse(SModBusFrame* frame);

    // dig for frames parameters
    quint32         m_currentIndex;
    EResponseState  m_rxState;
    SModBusFrame*   mp_rxFrame;
    QByteArray      m_rxRawBytes;
    quint32         m_crcState;

    // response parsing help parameters
    SModBusFrame    m_txFrame;
    QQueue<SModBusFrame> m_txFramesQueue;
    quint16         m_startAddr;

private slots:
    void on_readyRead();
};

#endif // CSMRM_H
