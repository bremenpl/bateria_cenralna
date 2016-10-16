#include "cbclogger.h"
#include "QDebug"

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
    // temp
    m_loggerStarted = true;
    m_verbose = verbose;
    m_fileName = filename;

    return 0;
}

/*
 * @brief   Adds the log line to the print queue.
 * @param   lvl: Log level of the line.
 * @param   text: Formatted input for va_list.
 */
void CBcLogger::print(ELogLevel lvl, const char* text, ...)
{
    // temp
    qDebug("test 1");
    qDebug() << "test 2";
}




