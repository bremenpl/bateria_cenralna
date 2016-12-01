#ifndef CSMRM_H
#define CSMRM_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QVector>
#include <QByteArray>

#define MAX_QUAN_OF_REGS            126

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
    qint32 sendRequest_ReadHoldingRegisters(const quint8 slaveId, const quint16 startAddr, const quint16 nrOfRegs);

signals:
    void responseReady_ReadHoldingRegisters(const quint8 slaveId, const QVector<quint16>& registers);
    void responeReady_ExceptionCode(const quint8 slaveId, EExceptionCodes code);

private:
    quint16 calculateCRC(const char *data, qint32 len);
    void addCrc2Frame(SModBusFrame& frame);
    qint32 sendFrame(const SModBusFrame& frame);
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

private slots:
    void on_readyRead();
};

#endif // CSMRM_H
