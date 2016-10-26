#include "cbclogger.h"

#include "QDebug"

#include "string.h"

// Initialize the instance pointer
CBcLogger* CBcLogger::mp_instance = NULL;

CBcLogger::CBcLogger()
{

}

/*
 * @brief   Singleton pattern instance return method.
 */
CBcLogger* CBcLogger::instance()
{
    // allow only one instance of the class
    if (!mp_instance)
    {
        mp_instance = new CBcLogger;
        Q_ASSERT(mp_instance);

        mp_instance->m_loggerStarted = false;
    }

    return mp_instance;
}

/*
 * @brief   Initializes the logger. After this method is called logging is possible.
 * @param   filename: File patch to the created log.
 * @param   verbose: Sets printing to console.
 * @return  0 if all ok.
 */
int CBcLogger::startLogger(QString filename, bool verbose)
{
    // register logline type
    qRegisterMetaType<logLine_t>("logLine_t");

    // start the printer thread
    m_printThread.start(QThread::IdlePriority);

    // move the printer object to it
    m_logPrinter.moveToThread(&m_printThread);

    // make the connection
    connect(mp_instance, SIGNAL(addNewLogLine(logLine_t)),
            &mp_instance->m_logPrinter, SLOT(onLogEventHappened(logLine_t)),
            Qt::QueuedConnection);

    // set the logger ftarted flag
    m_loggerStarted = true;

    return 0;
}

/*
 * @brief   Adds the log line to the print queue.
 * @param   lvl: Log level of the line.
 * @param   text: Formatted input for va_list.
 */
void CBcLogger::print(ELogLevel lvl, const char* text, ...)
{
    logLine_t logline;
    logline.loglvl = lvl;
    logline.datetime = QDateTime::currentDateTime();

    va_list argptr;
    va_start(argptr, text);

    char* output = NULL;
    if (vasprintf(&output, text, argptr))
    {
        logline.logstr = output;
        delete output;
    }

    va_end(argptr);

    emit addNewLogLine(logline);
}

void ClogPrinter::onLogEventHappened(const logLine_t& logline)
{
    qDebug() << "[" << logline.datetime.toString("dd.MM.yyyy-hh:mm:ss:zzz") << "]" <<
                "[" << "]";
}




