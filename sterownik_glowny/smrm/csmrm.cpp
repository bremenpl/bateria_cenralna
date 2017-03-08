#include "csmrm.h"
#include "cbclogger.h"
#include <QDebug>

Csmrm::Csmrm(QObject *parent) : QSerialPort(parent)
{
    qRegisterMetaType<QVector<quint16>>("QVector<quint16>");

    // create modbus specific connections
    connect(this, SIGNAL(readyRead()),
            this, SLOT(on_readyRead()), Qt::UniqueConnection);

    // set default params
    mp_rxFrame = 0;
    setResponseDefaultState();

    //connect(this, &Csmrm::newTxFrameEnqueued,
           // this, &Csmrm::on_newTxFrameEnqueued, Qt::QueuedConnection);
}

Csmrm::~Csmrm()
{
    // disconnect
    disconnect(this, SIGNAL(readyRead()),
               this, SLOT(on_readyRead()));
}

/*!
 * \brief Csmrm::convertArray2HexString: Converts a QByteArray to an hex values string
 * \param array: array to convert
 * \return string as space separated hex values
 */
const QString Csmrm::convertArray2HexString(const QByteArray& array)
{
    QString hexVals;

    foreach (quint8 byte, array)
    {
        //hexVals.append(QString::number(byte, 16));
        hexVals.append(QString("%1").arg(byte, 2, 16, QChar('0')));
        hexVals.append(" ");
    }

    return hexVals;
}

/*!
 * \brief Csmrm::on_readyRead: read all serial port data here
 */
void Csmrm::on_readyRead()
{
    // read the available data
    QByteArray rcvBuf(readAll());

    // find response
    digForResponse(rcvBuf);

    //CBcLogger::instance()->print(MLL::ELogLevel::LDebug)
            //<< "Received Data " << convertArray2HexString(rcvBuf);
}

/*!
 * \brief Csmrm::setResponseDefaultState: sets response parameters to default state
 */
void Csmrm::setResponseDefaultState()
{
    m_currentIndex = 0;
    m_rxState = EResponseState::SlaveAddr;
    m_crcState = 0;
    m_rxRawBytes.clear();
    if (mp_rxFrame) delete mp_rxFrame;
}

/*!
 * \brief Csmrm::digForResponse: Parses the received bytes and searches for a response
 * \param buf: Array containing response bytes.
 * \return non zero in case of error.
 */
qint32 Csmrm::digForResponse(const QByteArray& buf)
{
    foreach (quint8 byte, buf)
    {
        switch (m_rxState)
        {
            // TODO: handle wrong slave addresses
            case EResponseState::SlaveAddr:
            {
                mp_rxFrame =  new SModBusFrame;
                mp_rxFrame->slaveAddr = byte;
                m_currentIndex = 1;
                m_rxState = EResponseState::FuncCode;
                m_rxRawBytes.append(byte);
                break;
            }

            case EResponseState::FuncCode:
            {
                switch ((EFuncCodes)byte)
                {
                    case EFuncCodes::ReadHoldingRegisters:
                    case EFuncCodes::WriteSingleCoil:
                    {
                        mp_rxFrame->functionCode = (EFuncCodes)byte;
                        m_rxState = EResponseState::Data;
                        m_currentIndex = 2;
                        m_rxRawBytes.append(byte);
                        break;
                    }

                    default:
                    {
                        CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                                                     "Unhandled function code response (%u)", byte);
                        setResponseDefaultState();
                        return 2;
                    }
                }

                break;
            }

            case EResponseState::Data:
            {
                switch (mp_rxFrame->functionCode)
                {
                    case EFuncCodes::ReadHoldingRegisters:
                    {
                        if (2 == m_currentIndex)
                        {
                            // byte count (/2 = nr of registers)
                            mp_rxFrame->data.append(byte);
                            m_currentIndex = 3;
                            m_rxRawBytes.append(byte);
                        }
                        else
                        {
                            // register values
                            mp_rxFrame->data.append(byte);
                            m_currentIndex++;
                            m_rxRawBytes.append(byte);

                            // check when to move to crc phase
                            if ((m_currentIndex - 2) > (quint32)mp_rxFrame->data[0])
                                m_rxState = EResponseState::Crc;
                        }
                        break;
                    }

                    case EFuncCodes::WriteSingleCoil:
                    {
                        // byte count (/2 = nr of registers)
                        mp_rxFrame->data.append(byte);
                        m_currentIndex++;
                        m_rxRawBytes.append(byte);

                        // check when to move to crc phase
                        if (m_currentIndex >= 6)
                            m_rxState = EResponseState::Crc;
                    }

                    default: { }
                }

                break;
            }

            case EResponseState::Crc:
            {
                if (!m_crcState)
                {
                    m_crcState++;
                    m_currentIndex++;
                    mp_rxFrame->crc = (quint16)byte << 8;
                    m_rxRawBytes.append(byte);
                }
                else // last byte
                {
                    mp_rxFrame->crc |= (quint16)byte;
                    m_rxRawBytes.append(byte);

                    // check the crc
                    quint16 frameCrc = mp_rxFrame->crc;
                    addCrc2Frame(*mp_rxFrame);

                    // print raw data
                    CBcLogger::instance()->print(MLL::ELogLevel::LDebug)
                            << "MB RECV: " << convertArray2HexString(m_rxRawBytes);

                    if (frameCrc == mp_rxFrame->crc) // match
                        parseResponse(mp_rxFrame);
                    else
                    {
                        CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                                    "Bad CRC received: 0x%X, calculated: 0x%X", frameCrc, mp_rxFrame->crc);
                        setResponseDefaultState();
                        return 3;
                    }

                    setResponseDefaultState();
                }
                break;
            }

            default:
            {
                CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                                             "Unknown state while digging for frames (%u)", (quint32)m_rxState);
                setResponseDefaultState();
                return 1;
            }
        }
    }

    return 0;
}

/*!
 * \brief Csmrm::parseResponse: Parses the response and emits apropriate signal.
 * \param frame: pointer to a modbus frame
 */
void Csmrm::parseResponse(SModBusFrame* frame)
{
    Q_ASSERT(frame);

    // now emit proper signal
    switch (frame->functionCode)
    {
        case EFuncCodes::ReadHoldingRegisters:
        {
            // create a vector containing all registers
            QVector<quint16> hregs;
            quint32 nrOfRegs = (quint32)frame->data[0] / 2;
            quint16 reg = 0;

            for (quint32 i = 0, k = 1; i < nrOfRegs; i++, k += 2)
            {
                reg = (quint16)frame->data[k] << 8;
                reg |= (quint16)frame->data[k + 1];
                hregs.append(reg);
            }

            emit responseReady_ReadHoldingRegisters(frame->slaveAddr, m_startAddr, hregs);
            break;
        }

        case EFuncCodes::WriteSingleCoil:
        {
            // addr
            quint16 addr = ((quint16)frame->data[0] << 8) | frame->data[1];
            bool val = (bool)frame->data[2];

            emit responseReady_WriteSingleCoil(frame->slaveAddr, m_startAddr, val);
            break;
        }

        default:
        {
            CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                                         "Parsing unhandled function code (%u)", (quint32)frame->functionCode);
        }
    }
}

/*!
 * \brief Csmrm::sendFrame: Sends the specified frame through serial port
 * \param frame: refference to the frame 2 be sent.
 * \return 0 if sending failed, otherwise success.
 */
qint32 Csmrm::sendFrame(const SModBusFrame& frame)
{
    // check if device is open
    if (!isOpen())
        return 0;

    // all that needs to be done is to order the frame in bytes.
    QByteArray rb;

    rb.append(frame.slaveAddr);
    rb.append((quint8)frame.functionCode);
    rb.append(frame.data);
    rb.append((quint8)(frame.crc >> 8));
    rb.append((quint8)frame.crc);

    // send the data through serial port
    quint32 retVal = write(rb);

    // save start addr in case a frame needs it (ie. read holding regs)
    m_startAddr = (quint16)frame.data[0] << 8;
    m_startAddr |= (quint16)frame.data[1] & 0xFF;

    // print raw data
    if (retVal)
    {
        CBcLogger::instance()->print(MLL::ELogLevel::LDebug)
                << "MB SENT: " << convertArray2HexString(rb);
    }

    return retVal;
}

/*!
 * \brief enqueueFrame: Enqueues the frame for sending
 * \param frame
 * \return
 */
void Csmrm::enqueueFrame(const SModBusFrame& frame)
{
    m_txFramesQueue.enqueue(frame);
    //qDebug() << "queue++" << m_txFramesQueue.size();
}

int Csmrm::newTxFrameEnqueued()
{
    int size = m_txFramesQueue.size();

    if (size)
    {
        m_txFrame = m_txFramesQueue.dequeue();
        //qDebug() << "queue--" << m_txFramesQueue.size();

        if (!sendFrame(m_txFrame))
            CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                    << "Error sending data through modbus";
    }

    return size;
}

/*!
 * \brief Csmrm::sendRequest_ReadHoldingRegisters: Sends read holding registers request to specified slave
 * \param slaveId: Modbus slave id (1-247)
 * \param startAddr: read registers start address
 * \param nrOfRegs: number of registers to read (1-125)
 * \return
 */
void Csmrm::sendRequest_ReadHoldingRegisters(const quint8 slaveId, const quint16 startAddr, const quint16 nrOfRegs)
{
    // validate quantity of registers to read
    if ((nrOfRegs < 1) || (nrOfRegs > MAX_QUAN_OF_REGS))
        return;

    SModBusFrame frame;
    // assign slave id
    frame.slaveAddr = slaveId;

    // function code
    frame.functionCode = EFuncCodes::ReadHoldingRegisters;

    // data, starting address HI and LO
    frame.data.clear();
    frame.data.append((quint8)(startAddr >> 8));
    frame.data.append((quint8)startAddr);

    // data, no of registers HI and LO
    frame.data.append((quint8)(nrOfRegs >> 8)); // this is retarded, since max amount of registers is 125...
    frame.data.append((quint8)nrOfRegs);

    // calculate CRC
    addCrc2Frame(frame);

    // send the frame
    enqueueFrame(frame);
}

/*!
 * \brief Csmrm::sendRequest_ReadCoils Sends read coils request to specified slave
 * \param slaveId: Modbus slave id (1-247)
 * \param startAddr: read registers start address
 * \param nrOfRegs: number of registers to read (1-125)
 * \return
 */
void Csmrm::sendRequest_ReadCoils(const quint8 slaveId, const quint16 startAddr, const quint16 nrOfCoils)
{
    // validate quantity of coils to read
    if ((nrOfCoils < 1) || (nrOfCoils > MAX_QUAN_OF_COILS))
        return;

    SModBusFrame frame;
    // assign slave id
    frame.slaveAddr = slaveId;

    // function code
    frame.functionCode = EFuncCodes::ReadCoils;

    // data, starting address HI and LO
    frame.data.append((quint8)(startAddr >> 8));
    frame.data.append((quint8)startAddr);

    // data, no of coils HI and LO
    frame.data.append((quint8)(nrOfCoils >> 8));
    frame.data.append((quint8)nrOfCoils);

    // calculate CRC
    addCrc2Frame(frame);

    // send the frame
    enqueueFrame(frame);
}

/*!
 * \brief Csmrm::sendRequest_WriteSingleCoil Sends write single coil request to specified slave
 * \param slaveId: Modbus slave id (1-247)
 * \param addr: read registers start address
 * \param val: coil state
 */
void Csmrm::sendRequest_WriteSingleCoil(const uint8_t slaveId, const uint16_t addr, const bool val)
{
    SModBusFrame frame;
    // assign slave id
    frame.slaveAddr = slaveId;

    // function code
    frame.functionCode = EFuncCodes::WriteSingleCoil;

    // data, output address HI and LO
    frame.data.append((quint8)(addr >> 8));
    frame.data.append((quint8)addr);

    // data, value
    quint8 temp = 0;
    if (val)
        temp = 0xFF;

    frame.data.append(temp);
    temp = 0;
    frame.data.append(temp);

    // calculate CRC
    addCrc2Frame(frame);

    // send the frame
    enqueueFrame(frame);
}

/*!
 * \brief Csmrm::sendRequest_WriteSingleRegister
 * \param slaveId: Modbus slave id (1-247)
 * \param addr: read registers start address
 * \param val: register value
 */
void Csmrm::sendRequest_WriteSingleRegister(const uint8_t slaveId, const uint16_t addr, const uint16_t val)
{
    SModBusFrame frame;
    // assign slave id
    frame.slaveAddr = slaveId;

    // function code
    frame.functionCode = EFuncCodes::WriteSingleRegister;

    // data, address HI and LO
    frame.data.append((quint8)(addr >> 8));
    frame.data.append((quint8)addr);

    // data, value
    frame.data.append((quint8)(val >> 8));
    frame.data.append((quint8)val);

    // calculate CRC
    addCrc2Frame(frame);

    // send the frame
    enqueueFrame(frame);
}

/*!
 * \brief Csmrm::sendRequest_WriteMultipleRegisters
 * \param slaveId
 * \param startAddr
 * \param nrOfRegs
 * \param regs
 */
void Csmrm::sendRequest_WriteMultipleRegisters(const uint8_t slaveId,
                                               const quint16 startAddr,
                                               const quint16 nrOfRegs,
                                               const QVector<uint16_t> regs)
{
    SModBusFrame frame;
    // assign slave id
    frame.slaveAddr = slaveId;

    // function code
    frame.functionCode = EFuncCodes::WriteMultipleRegisters;

    // data, starting addraddress HI and LO
    frame.data.append((quint8)(startAddr >> 8));
    frame.data.append((quint8)startAddr);

    // data, quiantity of registers
    frame.data.append((quint8)(nrOfRegs >> 8));
    frame.data.append((quint8)nrOfRegs);

    // byte count
    frame.data.append((quint8)nrOfRegs * 2);

    // registers values
    foreach (auto reg, regs)
    {
        frame.data.append((quint8)(reg >> 8));
        frame.data.append((quint8)reg);
    }

    // calculate CRC
    addCrc2Frame(frame);

    // send the frame
    enqueueFrame(frame);
}


inline static quint16 crc_reflect(quint16 data, qint32 len)
{
    // Generated by pycrc v0.8.3, https://pycrc.org
    // Width = 16, Poly = 0x8005, XorIn = 0xffff, ReflectIn = True,
    // XorOut = 0x0000, ReflectOut = True, Algorithm = bit-by-bit-fast

    quint16 ret = data & 0x01;

    for (qint32 i = 1; i < len; i++)
    {
        data >>= 1;
        ret = (ret << 1) | (data & 0x01);
    }

    return ret;
}

/*!
 * \brief Csmrm::calculateCRC: Returns the CRC checksum of the first \a len bytes of \a data.
 * \param data: pointer to the data array.
 * \param len: length of the array (bytes)
 * \note The code used by the function was generated with pycrc. There is no copyright assigned
    to the generated code, however, the author of the script requests to show the line stating
    that the code was generated by pycrc (see implementation).
 * \return 16 bit CRC value
 */
quint16 Csmrm::calculateCRC(const char *data, qint32 len)
{
    // Generated by pycrc v0.8.3, https://pycrc.org
    // Width = 16, Poly = 0x8005, XorIn = 0xffff, ReflectIn = True,
    // XorOut = 0x0000, ReflectOut = True, Algorithm = bit-by-bit-fast

    quint16 crc = 0xFFFF;
    while (len--)
    {
        const quint8 c = *data++;
        for (qint32 i = 0x01; i & 0xFF; i <<= 1)
        {
            bool bit = crc & 0x8000;
            if (c & i)
                bit = !bit;

            crc <<= 1;
            if (bit)
                crc ^= 0x8005;
        }

        crc &= 0xFFFF;
    }

    crc = crc_reflect(crc & 0xFFFF, 16) ^ 0x0000;
    return (crc >> 8) | (crc << 8); // swap bytes
}


/*!
 * \brief Csmrm::addCrc2Frame: Puts the CRC in the frame
 * \param frame: refference to a frame object in which crc will be placed
 */
void Csmrm::addCrc2Frame(SModBusFrame& frame)
{
    // structure the struct in byte array
    QByteArray rb;

    rb.append(frame.slaveAddr);
    rb.append((quint8)frame.functionCode);
    rb.append(frame.data);

    // calc the crc
    frame.crc = calculateCRC(rb, rb.length());
}








