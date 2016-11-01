#include "cbclogger.h"
#include "string.h"

#include "QDebug"


ClogPrinter::ClogPrinter()
{
    // get the QMetaEnum object
    const QMetaObject &mo = MLL::staticMetaObject;
    int enum_index = mo.indexOfEnumerator("ELogLevel");
    m_metaEnum = mo.enumerator(enum_index);
}

void ClogPrinter::onLogEventHappened(const logLine_t& logline)
{
    QString constr = "[" + logline.datetime.toString("dd.MM.yyyy-hh:mm:ss:zzz") + "]" +
                     "[" + m_metaEnum.valueToKey(logline.loglvl) + "]" + " " + logline.logstr;

    if (m_verbose)
        vPrint() << constr << endl;
}

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
 * @param   ll: Set log level.
 */
void CBcLogger::startLogger(QString filename, bool verbose, MLL::ELogLevel ll)
{
    // register logline type
    qRegisterMetaType<logLine_t>("logLine_t");

    // start the printer thread
    m_printThread.start(QThread::IdlePriority);

    // move the printer object to it
    m_logPrinter.moveToThread(&m_printThread);

    // make the connection
    connect(mp_instance, SIGNAL(addNewLogLine(const logLine_t&)),
            &mp_instance->m_logPrinter, SLOT(onLogEventHappened(const logLine_t&)),
            Qt::QueuedConnection);

    // set the logger ftarted flag
    m_loggerStarted = true;

    // set the verbose flag
    m_logPrinter.setVerbose(verbose);

    // log level threshold
    setLogLevel(ll);
}

/*!
 * \brief Adds the log line to the print queue, va_list style.
 * \param lvl: Log level of the line.
 * \param text: Formatted input for va_list.
 */
void CBcLogger::print(MLL::ELogLevel lvl, const char* text, ...)
{
    // check if logger initialized
    if (!m_loggerStarted)
        return;

    // check if log level sufficient
    if (lvl > m_setLogLvl)
        return;

    logLine_t logline;
    logline.loglvl = lvl;
    logline.datetime = QDateTime::currentDateTime();

    va_list argptr;
    va_start(argptr, text);

    char* output = NULL;
    if (vasprintf(&output, text, argptr))
    {
        logline.logstr = output;
        free(output);
    }

    va_end(argptr);
    emit addNewLogLine(logline);
}

/*!
 * \brief Adds the log line to the print queue, stream style.
 * \param lvl: Log level of the line.
 * \return \ref LoggerHelper to print the stream
 */
LoggerHelper CBcLogger::print(MLL::ELogLevel lvl) const
{
    return LoggerHelper(lvl, m_setLogLvl, m_loggerStarted);
}

/*!
 * \brief Operator definition
 * \return Logging object
 */
LoggerHelper CBcLogger::operator()() const { return LoggerHelper{}; }






