#include "cbcserialthread.h"


/*!
 * \brief CBcSerialThread::CBcSerialThread Class only constructor
 * \param port: serial port name to be associated with modbus RTU master
 * \param parent: parent object
 */
CBcSerialThread::CBcSerialThread(const QString& port, QObject *parent) : QThread(parent)
{
    // Create new modbus RTU client object
    mp_modbusDevice = new QModbusRtuSerialMaster(this);

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QString pname = info.portName();
        if (pname == port)
        {
            mp_modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter, port);
            break;
        }
    }

    if ((mp_modbusDevice->connectionParameter(QModbusDevice::SerialPortNameParameter) != port) ||
        ("" == port))
    {
        CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                << "Serial port " << port << " not found, quitting";

        // throw exception code and return
        delete mp_modbusDevice;
        throw -1;
        return;
    }

    // port set correctly, continue
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo)
            << "Serial port " << port << "found. Set as ModBus RTU master device";
}

/*!
 * \brief CBcSerialThread::~CBcSerialThread class destructor. Closes all connections and removes objects.
 */
CBcSerialThread::~CBcSerialThread()
{
    // disconnect modbus rtu master
    if (mp_modbusDevice)
    {
        mp_modbusDevice->disconnectDevice();
        delete mp_modbusDevice;
    }
}

/*!
 * \brief CBcSerialThread::run: The thread run function
 */
void CBcSerialThread::run()
{

}








