#include "cbcmain.h"

/*!
 * \brief CBcMain::CBcMain: Constructor of Main class. All code should happen in here.
 *        This abstract class is created in order to use qt meta functionalities.
 * \param coreApp: refference to a core application object. Needed for input parameters.
 * \param parent
 */
CBcMain::CBcMain(QCoreApplication& coreApp, QObject *parent) : QObject(parent)
{
    mp_coreApp = &coreApp;

    // check parameters
    int retVal = getParameters(coreApp, m_verbose, m_path, m_ll, m_serialName);
    if (0 != retVal)
    {
        if (-1 == retVal)
            qCritical("Serial port not provided as parameter. Quiting");
        else
            qCritical("Parsing user parameters failed (%i). Aborting.", retVal);

        return;
    }

    // start the logger
    CBcLogger::instance()->startLogger(m_path, m_verbose, m_ll);

    // start modbus rtu master
    try
    {
        mp_serialThread = new CBcSerialThread(m_serialName);
        mp_serialThread->moveToThread(mp_serialThread);
        mp_serialThread->start(QThread::HighPriority);
    }
    catch (int retVal)
    {
        QThread::usleep(500);
        return;
    }

    // start TCP server
    m_tcpServer.startServer();

    // connect status change signal
    connect(mp_serialThread, SIGNAL(statusChanged(quint16)),
            &m_tcpServer, SLOT(on_modbusStatusChanged(quint16)), Qt::DirectConnection);

    // hang in here
    mp_coreApp->exec();
}

/*!
 * \brief CBcMain::~CBcMain: Class destructor, try do delete all possible objects in here.
 */
CBcMain::~CBcMain()
{
    if (mp_serialThread)
    {
        //mp_serialThread->exit(0);
        //delete mp_serialThread;
        //mp_serialThread->deleteLater();
    }
}

/*!
 * \brief Check the \ref coreApp parameters and sets appropriate flags and strings.
 * \param coreApp: Refference to a \ref QCoreApplication variable containing parameters.
 * \param verbose: Refference under which verbose flag will be saved.
 * \param logPath: Refference under which logs directory path will be saved.
 * \param logLevel: Refference under which the log level for the logger will be saved.
 * \return 0 if all ok.
 */
int CBcMain::getParameters(QCoreApplication& coreApp,
                           bool& verbose,
                           QString& logPath,
                           MLL::ELogLevel& logLevel,
                           QString& serial)
{
    int retVal = 0;

    // set app info
    coreApp.setApplicationName(APP_NAME);
    coreApp.setApplicationVersion(APP_VER);

    // initialize parser
    QCommandLineParser parser;
    parser.setApplicationDescription(APP_DESC);
    parser.addHelpOption();
    parser.addVersionOption();

    // add options
    QCommandLineOption verboseOpt(QStringList() << "c" << "console",
            QCoreApplication::translate("main", "Print logs to console."));
    if (!parser.addOption(verboseOpt))
        retVal++;

    QCommandLineOption pathOpt(QStringList() << "p" << "path",
            QCoreApplication::translate("main", "Path to the location in which logs will be saved."),
            QCoreApplication::translate("main", "location"));
    if (!parser.addOption(pathOpt))
        retVal++;

    QCommandLineOption serialOpt(QStringList() << "s" << "serial",
            QCoreApplication::translate("main", "Serial port device name, eg. ttyAMA0."),
            QCoreApplication::translate("main", "device"));
    if (!parser.addOption(serialOpt))
        retVal++;

    QCommandLineOption logLevelOpt(QStringList() << "l" << "level",
            QCoreApplication::translate("main", "Sets the logging level (0-4)."),
            QCoreApplication::translate("main", "level"));
   if (!parser.addOption(logLevelOpt))
       retVal++;

    // process the parameters
    parser.process(coreApp);

    // collect parameters
    verbose = parser.isSet(verboseOpt);
    logPath = parser.value(pathOpt);
    serial = parser.value(serialOpt);
    logLevel = (MLL::ELogLevel)(parser.value(logLevelOpt).toInt());

    // trim log level
    if (logLevel > MLL::LDebug)
        logLevel = MLL::LDebug;

    // check if path is proper
    if ("" == logPath)
        logPath = QDir::tempPath();

    // check if serial port device was set
    if (!parser.isSet("serial"))
        retVal = -1;

    return retVal;
}
