#include "cbcmain.h"

/*!
 * \brief CBcMain::CBcMain: Constructor of Main class. All code should happen in here.
 *        This abstract class is created in order to use qt meta functionalities.
 * \param coreApp: refference to a core application object. Needed for input parameters.
 * \param parent
 */
CBcMain::CBcMain(QCoreApplication& coreApp, QObject *parent) : QObject(parent)
{
    // check parameters
    int retVal = getOrCteareSettings(coreApp);
    if (0 != retVal)
    {
        if (-1 == retVal)
            m_serialsValid = false;
        else
        {
            qCritical("Parsing user parameters failed (%i). Aborting.", retVal);
            return;
        }
    }
    else
        m_serialsValid = true;

    // start the logger
    CBcLogger::instance()->startLogger(m_logPath, m_verbose, m_ll);

    // start modbus rtu master only if the serial port is valid
    if (m_serialsValid)
    {
        try
        {
            mp_lcSerialThread = new CBcLcThread(m_lcSerialName, m_noOfPings, m_noOfLcSlaves);
            mp_lcSerialThread->moveToThread(mp_lcSerialThread);
            mp_lcSerialThread->start(QThread::HighPriority);

            // connect status change signal
            connect(mp_lcSerialThread, SIGNAL(statusChanged(quint16)),
                    &m_tcpServer, SLOT(on_modbusStatusChanged(quint16)), Qt::DirectConnection);
        }
        catch (int retVal)
        {
            QThread::usleep(500);
            return;
        }
    }
    else
        CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "No serial port for LC provided");

    // start TCP server
    m_tcpServer.startServer();

    // hang in here
    coreApp.exec();
}

/*!
 * \brief CBcMain::~CBcMain: Class destructor, try do delete all possible objects in here.
 */
CBcMain::~CBcMain()
{
    if (mp_lcSerialThread)
    {
        //mp_serialThread->exit(0);
        //delete mp_serialThread;
        //mp_serialThread->deleteLater();
    }
}

void CBcMain::readSettings()
{
    // parse parameters
    // logger section
    mp_settings->beginGroup("logger");
    m_logPath = mp_settings->value("path").toString();
    quint32 temp = mp_settings->value("level").value<quint32>();
    m_ll = (MLL::ELogLevel)temp;
    m_verbose = mp_settings->value("verbose").value<bool>();
    mp_settings->endGroup();

    // serial ports
    mp_settings->beginGroup("serial");
    m_noOfPings = mp_settings->value("scansnr").value<quint32>();
    m_noOfLcSlaves = mp_settings->value("lcnr").value<quint32>();
    m_lcSerialName = mp_settings->value("lcserial").value<QString>();
    mp_settings->endGroup();
}

/*!
 * \brief CBcMain::getOrCteareSettings
 * \param coreApp
 * \return
 */
int CBcMain::getOrCteareSettings(QCoreApplication& coreApp)
{
    int retVal = 0;

    // set app info
    coreApp.setApplicationName(APP_NAME);
    coreApp.setApplicationVersion(APP_VER);

    // set application path
    m_settingsPath = QCoreApplication::applicationDirPath() + "/core_settings.ini";
    mp_settings = new QSettings(m_settingsPath, QSettings::IniFormat);

    // check if file exists
    QFileInfo check_file(m_settingsPath);
    if (!(check_file.exists() && check_file.isFile()))
    {
        // it does not, create it
        // logger
        mp_settings->beginGroup("logger");
        mp_settings->setValue("path", "/tmp"); // default logs path
        mp_settings->setValue("level", (quint32)MLL::LDebug); // default logging level
        mp_settings->setValue("verbose", 1); // default verbose flag ON
        mp_settings->endGroup();

        // serial ports
        mp_settings->beginGroup("serial");
        mp_settings->setValue("scansnr", 3); // default number of scans for presence
        mp_settings->setValue("lcserial", "none"); // no line controllers serial provided
        mp_settings->setValue("lcnr", 64); // default line controller slaves
        mp_settings->setValue("batserial", "none"); // no batteries serial provided
        mp_settings->setValue("batnr", 6); // default batteries controller slaves
        mp_settings->endGroup();
    }

    //read the settings
    readSettings();

    // check serial
    if ("none" == m_lcSerialName)
        retVal = -1;

    return retVal;
}









