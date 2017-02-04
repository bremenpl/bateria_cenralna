#include "cbclogger.h"
#include "string.h"

#include <QDebug>

#ifdef _WIN32
#define insane_free(ptr) { free(ptr); ptr = 0; } // cool macro!

int vasprintf(char **strp, const char *fmt, va_list ap)
{
  int r = -1, size;

  va_list ap2;
  va_copy(ap2, ap);

  size = vsnprintf(0, 0, fmt, ap2);

  if ((size >= 0) && (size < INT_MAX))
  {
    *strp = (char *)malloc(size+1); //+1 for null
    if (*strp)
    {
      r = vsnprintf(*strp, size+1, fmt, ap);  //+1 for null
      if ((r < 0) || (r > size))
      {
        insane_free(*strp);
        r = -1;
      }
    }
  }
  else { *strp = 0; }

  va_end(ap2);

  return(r);
}
#endif


ClogPrinter::ClogPrinter()
{
    // get the QMetaEnum object
    const QMetaObject &mo = MLL::staticMetaObject;
    int enum_index = mo.indexOfEnumerator("ELogLevel");
    m_metaEnum = mo.enumerator(enum_index);
    mp_fss = 0;
}

ClogPrinter::~ClogPrinter()
{
    if (mp_fss)
        delete mp_fss;
}

/*!
 * \brief Creates the log file for this session and opens it.
 * \param path: path to directory where file wil be created
 * \return 0 if all ok.
 */
int ClogPrinter::createNewLogFile(QString& path)
{
    // create file name
    QDateTime dateTime(QDateTime::currentDateTime());
    QString filename = path + "/" + dateTime.toString("dd.MM.yyyy-hh.mm.ss.zzz") + ".log";

    // create and open file
    m_logFile.setFileName(filename);
    if (m_logFile.open(QFile::WriteOnly | QFile::Append))
    {
        // set stream
        mp_fss = new QTextStream(&m_logFile);
        return 0;
    }

    return 1;
}

void ClogPrinter::onLogEventHappened(const logLine_t& logline)
{
    // construct string
    QString constr = "[" + logline.datetime.toString("dd/MM/yyyy-hh:mm:ss.zzz") + "]" +
                     "[" + m_metaEnum.valueToKey(logline.loglvl) + "] " + logline.logstr;

    // print to console if verbose set
    if (m_verbose)
        vPrint() << constr << endl;

    // save to specified file
    if (mp_fss)
        *mp_fss << constr << endl;
}

// Initialize the instance pointer
CBcLogger* CBcLogger::mp_instance = NULL;

CBcLogger::CBcLogger()
{

}

CBcLogger::~CBcLogger()
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

/*!
 * \brief Initializes the logger. After this method is called logging is possible.
 * \param fileDir: File patch to the created log.
 * \param verbose: Sets printing to console.
 * \param ll: Set log level.
 */
void CBcLogger::startLogger(QString fileDir, bool verbose, MLL::ELogLevel ll)
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

    // create log file
    int ret = m_logPrinter.createNewLogFile(fileDir);
    if (ret)
    {
        qCritical() << "Unable to create file in directory" << fileDir << ". Aborting.";
        exit(ret);
    }
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






