#include "cbcserialthread.h"


/*!
 * \brief CBcSerialThread::CBcSerialThread Class only constructor
 * \param port: serial port name to be associated with modbus RTU master
 * \param parent: parent object
 */
CBcSerialThread::CBcSerialThread(const QString& port,
                                 const quint32 noOfPings,
                                 const quint32 noOfDevices,
                                 QObject *parent) : QThread(NULL)
{
    // create modbus master
    mp_modbusMaster =  new Csmrm(this);
    m_curScanDev = 0;
    m_pingsForPresence = noOfPings;
    m_noOfDev2Scan = noOfDevices;
    m_slaves.clear();

    // set serial parameters
    mp_modbusMaster->setPortName(port);
    mp_modbusMaster->setBaudRate(QSerialPort::Baud9600);
    mp_modbusMaster->setDataBits(QSerialPort::Data8);
    mp_modbusMaster->setParity(QSerialPort::NoParity);
    mp_modbusMaster->setStopBits(QSerialPort::OneStop);
    mp_modbusMaster->setFlowControl(QSerialPort::NoFlowControl);

    // connect response slots
    connect(mp_modbusMaster, SIGNAL(responseReady_ReadHoldingRegisters(
                                        const quint8, const quint16, const QVector<quint16>&)),
            this, SLOT(on_responseReady_ReadHoldingRegisters(
                           const quint8, const quint16, const QVector<quint16>&)),
            Qt::UniqueConnection);

    connect(mp_modbusMaster, SIGNAL(responseReady_ReadCoils(const quint8, const quint16, const QVector<bool>&)),
            this, SLOT(on_responseReady_ReadCoils(const quint8, const quint16, const QVector<bool>&)),
            Qt::UniqueConnection);

    connect(mp_modbusMaster, &Csmrm::responseReady_WriteSingleCoil,
            this, &CBcSerialThread::on_responseReady_WriteSingleCoil, Qt::UniqueConnection);

    // tcp slots
    if (parent)
    {
        CBcTcpServer* server = dynamic_cast<CBcTcpServer*>(parent);

        connect(server, &CBcTcpServer::sendData2ModbusSlave,
                this, &CBcSerialThread::on_sendData2ModbusSlave, Qt::QueuedConnection);
    }

    // try to open the port
    if (!mp_modbusMaster->open(QIODevice::ReadWrite))
    {
         CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                 << "Cannot open serial port " << mp_modbusMaster->portName();
         throw -1;
    }

    CBcLogger::instance()->print(MLL::ELogLevel::LInfo)
            << mp_modbusMaster->portName() << " Opened";
}

bool CBcSerialThread::reOpenPort()
{
    mp_modbusMaster->close();
    if (!mp_modbusMaster->open(QIODevice::ReadWrite))
    {
         CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                 << "Cannot open serial port " << mp_modbusMaster->portName();
         return false;
    }

    CBcLogger::instance()->print(MLL::ELogLevel::LInfo)
            << "Serial port " << mp_modbusMaster->portName() << " reopened";
    return true;
}

/*!
 * \brief CBcSerialThread::~CBcSerialThread class destructor. Closes all connections and removes objects.
 */
CBcSerialThread::~CBcSerialThread()
{
    if (mp_modbusMaster)
    {
        mp_modbusMaster->close();
        mp_modbusMaster->deleteLater();
    }

    if (mp_pollTimer)
        mp_pollTimer->deleteLater();

    if (mp_respToutTimer)
        mp_respToutTimer->deleteLater();

    while (m_slaves.size())
        m_slaves.last()->deleteLater();
}

/*!
 * \brief CBcSerialThread::on_setPingParameters
 * \param noOfPings
 * \param noOfDevices
 */
void CBcSerialThread::on_setPingParameters(const quint32 noOfPings, const quint32 noOfDevices)
{
    m_pingsForPresence = noOfPings;
    m_noOfDev2Scan = noOfDevices;
}

/*!
 * \brief CBcSerialThread::on_responseReady_ReadHoldingRegisters
 * \param slaveId
 * \param startAddr
 * \param registers
 */
void CBcSerialThread::on_responseReady_ReadHoldingRegisters(const quint8 slaveId,
                                                            const quint16 startAddr,
                                                            const QVector<quint16>& registers)
{
    // turn off response timer
    mp_respToutTimer->stop();
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                 "Read holding registers response. SID:%u, SADDR:%u, NOOFREGS:%u",
                                 slaveId, startAddr, registers.size());

    // check if this is ping
    switch ((EAddrCodesLC)startAddr)
    {
        case EAddrCodesLC::Ping:
        {
            // manage
            m_slaves[slaveId - 1]->managePresence(true);
            break;
        }

        case EAddrCodesLC::UniqId:
        {
            m_slaves[slaveId - 1]->sendGetCmdToClients(tcpCmd::takeUniqId, registers);
            break;
        }

        default:
        {

        }
    }

    // slave device related command. Different for LC and different for BAT.
    responseReady_ReadHoldingRegistersOverride(slaveId, startAddr, registers);
    mp_pollTimer->start();
}

void CBcSerialThread::on_responseReady_WriteSingleCoil(const quint8 slaveId,
                                                       const quint16 addr,
                                                       const bool val)
{
    mp_respToutTimer->stop();
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                 "Write single coil response. SID:%u, SADDR:%u, STATE:%u",
                                 slaveId, addr, val);

    responseReady_WriteSingleCoil(slaveId, addr, val);
    mp_pollTimer->start();
}

/*!
 * \brief CBcSerialThread::responseReady_ReadHoldingRegistersOverride
 *        Inheriting classes should place their code here
 * \param slaveId
 * \param startAddr
 * \param registers
 */
void CBcSerialThread::responseReady_ReadHoldingRegistersOverride(const quint8 slaveId,
                                                        const quint16 startAddr,
                                                        const QVector<quint16>& registers)
{
    CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
        "Read holding registers response NOT OVERRIDEN. SID:%u, SADDR:%u, NOOFREGS:%u",
         slaveId, startAddr, registers.size());
}

void CBcSerialThread::responseReady_WriteSingleCoil(const quint8 slaveId,
                                                    const quint16 addr,
                                                    const bool val)
{
    CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
        "Write single coil response NOT OVERRIDEN. SID:%u, SADDR:%u, STATE:%u",
        slaveId, addr, val);
}

void CBcSerialThread::on_responseReady_ReadCoils(const quint8 slaveId,
                                                 const quint16 startAddr,
                                                 const QVector<bool>& coils)
{
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                 "Read coils response. SID:%u, SADDR:%u, NOOFCOILS:%u",
                                 slaveId, startAddr, coils.size());

}

void CBcSerialThread::on_sendData2ModbusSlave(const tcpReq req,
                             const tcpCmd cmd,
                             const QVector<slaveId*>& pv,
                             const QByteArray& data)
{
    (void)data;

    switch (pv.first()->m_slaveType)
    {
        case EDeviceTypes::LineCtrler:
        {
            m_rcOffset = 0;
            if (pv.size() > 1)
                m_rcOffset = pv.last()->m_slaveAddr * 0xFF; // ie 0x100 is addr 1 for RC 1

            switch (cmd)
            {
                case tcpCmd::takeUniqId:
                {
                    if (req != tcpReq::get)
                        CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                                                     "Cant write read only register takeUniqId!");
                    else
                    {
                        // read registers
                        mp_modbusMaster->sendRequest_ReadHoldingRegisters(pv.first()->m_slaveAddr,
                            (quint16)EAddrCodesLC::UniqId + m_rcOffset, NO_OF_UNIQDID_REG);
                    }
                    break;
                }

                case tcpCmd::takeStatus:
                {
                    if (req != tcpReq::get)
                        CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                                                     "Cant write read only register takeStatus!");
                    else
                    {
                        // read registers
                        mp_modbusMaster->sendRequest_ReadHoldingRegisters(pv.first()->m_slaveAddr,
                            (quint16)EAddrCodesRC::Status + m_rcOffset, NO_OF_STATUS_REGS);
                    }
                    break;
                }

                case tcpCmd::setRcBit:
                {
                    if (req != tcpReq::set)
                        CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                                                    "Cant read write only coil setRcBit!");
                    else
                    {
                        // write single coil
                        mp_modbusMaster->sendRequest_WriteSingleCoil(pv.first()->m_slaveAddr,
                            (quint16)ECoilAddrCodesRc::Relay + m_rcOffset, data[0]);
                    }
                    break;
                }

                default:
                {
                    CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                        "Cannot send serial data for unknown cmd (%i)", (int)cmd);
                }
            }
            break;
        }

        default:
        {
            CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                "Cannot send serial data to unknown slave type (%i)", (int)pv.first()->m_slaveType);
        }
    }
}

/*!
 * \brief CBcSerialThread::on_pollTimeout
 *        In this thread pinging happens.
 */
void CBcSerialThread::on_pollTimeout()
{
    // ping only if no more messages pending
    if (!mp_modbusMaster->txFrameQueue()->size())
    {
        if (m_curPing < MAX_PINGS)
        {
            m_curPing++;

            // check slave overflow, scan one slave at a time
            if (m_curScanDev >= m_noOfDev2Scan)
                m_curScanDev = 1;
            else
                m_curScanDev++;

            // read registers
            mp_modbusMaster->sendRequest_ReadHoldingRegisters(
                m_curScanDev, (quint16)EAddrCodesLC::Ping, NO_OF_PING_REGS);
        }
    }

    // dequeue and send
    if (mp_modbusMaster->newTxFrameEnqueued())
        mp_respToutTimer->start(); // start response timeout timer
    else
        mp_pollTimer->start();
}

/*!
 * \brief CBcSerialThread::on_respTimeout
 * If this function is called response timeout occured
 */
void CBcSerialThread::on_respTimeout()
{
    // check if this was ping
    if (Csmrm::EFuncCodes::ReadHoldingRegisters == mp_modbusMaster->txFrame().functionCode)
    {
        // ping address
        quint16 addr = (mp_modbusMaster->txFrame().data[0] << 8) | mp_modbusMaster->txFrame().data[1];
        if ((quint16)EAddrCodesLC::Ping == addr)
        {
            // proper slave
            if (m_curScanDev == mp_modbusMaster->txFrame().slaveAddr)
            {
                // manage
                if (m_slaves[mp_modbusMaster->txFrame().slaveAddr - 1]->managePresence(false))
                    reOpenPort();
            }
            else
            {
                CBcLogger::instance()->print(MLL::ELogLevel::LWarning,
                    "Wrong slave answered in ping %u, should be %u",
                    mp_modbusMaster->txFrame().slaveAddr, m_curScanDev);
            }
        }
    }

    // all ok just start
    mp_pollTimer->start();
}

/*!
 * \brief CBcSerialThread::run: The thread run function
 */
void CBcSerialThread::run()
{
    // create poll timer
    mp_pollTimer = new QTimer();

    // create connections
    connect(mp_pollTimer, SIGNAL(timeout()),
            this, SLOT(on_pollTimeout()), Qt::UniqueConnection);

    // create timeout timer
    mp_respToutTimer = new QTimer();

    // connect it
    connect(mp_respToutTimer, SIGNAL(timeout()),
            this, SLOT(on_respTimeout()), Qt::UniqueConnection);

    // set resp timeout timer
    mp_respToutTimer->setSingleShot(true);
    mp_respToutTimer->setInterval(1000);

    // set and start poll timer
    mp_pollTimer->setSingleShot(true);
    mp_pollTimer->setInterval(300);
    mp_pollTimer->start();

    exec();
}


















