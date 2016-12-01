#include "cbcserialthread.h"


/*!
 * \brief CBcSerialThread::CBcSerialThread Class only constructor
 * \param port: serial port name to be associated with modbus RTU master
 * \param parent: parent object
 */
CBcSerialThread::CBcSerialThread(const QString& port, QObject *parent) : QThread(parent)
{
    // create modbus master
    mp_modbusMaster =  new Csmrm(this);

    // set serial parameters
    mp_modbusMaster->setPortName(port);
    mp_modbusMaster->setBaudRate(QSerialPort::Baud9600);
    mp_modbusMaster->setDataBits(QSerialPort::Data8);
    mp_modbusMaster->setParity(QSerialPort::EvenParity);
    mp_modbusMaster->setStopBits(QSerialPort::OneStop);
    mp_modbusMaster->setFlowControl(QSerialPort::NoFlowControl);

    // connect response slots
    connect(mp_modbusMaster, SIGNAL(responseReady_ReadHoldingRegisters(const quint8, const QVector<quint16>&)),
            this, SLOT(on_responseReady_ReadHoldingRegisters(const quint8, const QVector<quint16>&)),
            Qt::UniqueConnection);

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
}

/*!
 * \brief CBcSerialThread::on_responseReady_ReadHoldingRegisters
 * \param slaveId
 * \param registers
 */
void CBcSerialThread::on_responseReady_ReadHoldingRegisters(const quint8 slaveId, const QVector<quint16>& registers)
{
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Read holding registers response arrived from slave %u", slaveId);

    // now send the response to connected clients
    emit statusChanged(registers.last());
}

/*!
 * \brief CBcSerialThread::on_pollTimeout
 */
void CBcSerialThread::on_pollTimeout()
{
    int temp = mp_modbusMaster->sendRequest_ReadHoldingRegisters(5, 0, 1);
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

    mp_pollTimer->setSingleShot(false);
    mp_pollTimer->setInterval(1000);
    mp_pollTimer->start();

    exec();
}


















