#include <QCoreApplication>
#include <QCommandLineParser>
#include <QString>
#include <QDebug>
#include <QDir>

#include "literals.h"
#include "cbclogger.h"
#include "cbcserialthread.h"

int getParameters(QCoreApplication& coreApp, bool& verbose, QString& logPath, MLL::ELogLevel& logLevel, QString& serial);

int main(int argc, char *argv[])
{
    QCoreApplication coreApp(argc, argv);

    // parameters
    bool verbose;
    QString path;
    MLL::ELogLevel ll;
    QString serialName;

    int retVal = getParameters(coreApp, verbose, path, ll, serialName);
    if (0 != retVal)
    {
        if (-1 == retVal)
            qCritical("Serial port not provided as parameter. Quiting");
        else
            qCritical("Parsing user parameters failed (%i). Aborting.", retVal);
        return retVal;
    }

    // logger start
    CBcLogger::instance()->startLogger(path, verbose, ll);

    // start modbus rtu master
    try
    {
        CBcSerialThread serialThread(serialName);
        serialThread.moveToThread(&serialThread);
    }
    catch (int retVal)
    {
        QThread::usleep(500);
        return retVal;
    }

    //CBcLogger::instance()->print(MLL::ELogLevel::LCritical, "abc %u 0x%X", 5, 99);
    //CBcLogger::instance()->print(MLL::ELogLevel::LCritical) << "test " << 123 << " 345";
    return coreApp.exec();
}

/*!
 * \brief Check the \ref coreApp parameters and sets appropriate flags and strings.
 * \param coreApp: Refference to a \ref QCoreApplication variable containing parameters.
 * \param verbose: Refference under which verbose flag will be saved.
 * \param logPath: Refference under which logs directory path will be saved.
 * \param logLevel: Refference under which the log level for the logger will be saved.
 * \return 0 if all ok.
 */
int getParameters(QCoreApplication& coreApp, bool& verbose, QString& logPath, MLL::ELogLevel& logLevel, QString& serial)
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
