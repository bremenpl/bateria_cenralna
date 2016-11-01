#include <QCoreApplication>
#include <QCommandLineParser>
#include <QString>
#include <QDebug>

#include "literals.h"
#include "cbclogger.h"

int getParameters(QCoreApplication& coreApp, bool& verbose, QString& logPath, MLL::ELogLevel& logLevel);

int main(int argc, char *argv[])
{
    QCoreApplication coreApp(argc, argv);

    // parameters
    bool verbose;
    QString path;
    MLL::ELogLevel ll;

    int retVal = getParameters(coreApp, verbose, path, ll);
    if (0 != retVal)
    {
        qCritical("Parsing user parameters failed (%i). Aborting.", retVal);
        return retVal;
    }

    // logger tests
    CBcLogger::instance()->startLogger("test", true, MLL::ELogLevel::LDebug);
    CBcLogger::instance()->print(MLL::ELogLevel::LDebug, "abc %u 0x%X", 5, 99);
    LoggerHelper(MLL::ELogLevel::LInfo) << "test";


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
int getParameters(QCoreApplication& coreApp, bool& verbose, QString& logPath, MLL::ELogLevel& logLevel)
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
    logLevel = (MLL::ELogLevel)(parser.value(logLevelOpt).toInt());

    return retVal;
}
