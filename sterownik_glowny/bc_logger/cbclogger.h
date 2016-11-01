#ifndef CBCLOGGER_H
#define CBCLOGGER_H

#include <QMetaEnum>
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QThread>
#include <QTextStream>
#include <QFile>

/*
 * @brief   Describes the log level for an item
 */
#ifndef Q_MOC_RUN
namespace MLL
#else
class MLL
#endif
{
#if defined(Q_MOC_RUN)
    Q_GADGET
    Q_ENUMS(ELogLevel)
public:
#endif
    enum ELogLevel
    {
        LNone = 0,
        LCritical,
        LWarning,
        LInfo,
        LDebug
    };
    extern const QMetaObject staticMetaObject;
}

/*
 * @brief   Single log object descriptor
 */
struct logLine_t
{
    QString             logstr;         /*!< String to be saved to file */
    MLL::ELogLevel      loglvl;         /*!< Log level for an item */
    QDateTime           datetime;       /*!< Time stamp for the log item */
};

/*
 * @brief   Object is created in order to work in a separate thread. In this thread slot is called with queue
 *          in order to print the logs.
 */
class ClogPrinter : public QObject
{
    Q_OBJECT
public:
    ClogPrinter();
    ~ClogPrinter();
    void setVerbose(bool val = false) { m_verbose = val; }
    int createNewLogFile(QString& path);

public slots:
    void onLogEventHappened(const logLine_t& logline);

private:
    // methods
    inline QTextStream& vPrint()
    {
        static QTextStream r{stdout};
        return r;
    }

    // members
    QMetaEnum       m_metaEnum;             /*!< MetaEnum object used for serialising log levels */
    bool            m_verbose;              /*!< Defines either logs should be also printed to console */
    QTextStream*    mp_fss;                 /*!< To file save stream */
    QFile           m_logFile;              /*!< Log file */
};

struct LoggerHelper;

class CBcLogger : public QObject
{
    Q_OBJECT
public:
    // methods
    //explicit CBcLogger(QObject *parent = 0);
    static CBcLogger* instance();

    void startLogger(QString fileDir, bool verbose, MLL::ELogLevel ll);
    void setLogLevel(MLL::ELogLevel ll = MLL::LDebug) { m_setLogLvl = ll; }

    void print(MLL::ELogLevel lvl, const char* text, ...);
    LoggerHelper print(MLL::ELogLevel lvl) const;
    LoggerHelper operator()() const;


signals:
    void addNewLogLine(const logLine_t& logline);

private:
// members
    static CBcLogger*   mp_instance;
    MLL::ELogLevel      m_setLogLvl;            /*!< Log level to compare */
    bool                m_loggerStarted;        /*!< logging possible only if set */
    QThread             m_printThread;          /*!< Printer object has to be moved to this thread */
    ClogPrinter         m_logPrinter;           /*!< Log printer object */

// methods
    CBcLogger();                              /*!< Hidden constructor cannot be called */
    ~CBcLogger();

// operators
    //CBcLogger* &operator<<(const char *c);

public slots:
};

struct LoggerHelper
{
private:
    bool m_noPrint;
    logLine_t m_logline;

public:
    explicit LoggerHelper(MLL::ELogLevel ll = MLL::LDebug,
                          MLL::ELogLevel llmax = MLL::LNone,
                          bool logStarted = false)
    {
        if ((!logStarted) || (ll > llmax))
            m_noPrint = true;
        else
        {
            m_noPrint = false;
            m_logline.datetime = QDateTime::currentDateTime();
            m_logline.loglvl = ll;
        }
    }
    //LoggerHelper(LoggerHelper&&) = default;

    ~LoggerHelper()
    {
        if (!m_noPrint)
            emit CBcLogger::instance()->addNewLogLine(m_logline);
    }

    template<typename T>
    LoggerHelper& operator<<(T const& val)
    {
        if (!m_noPrint)
        {
            QTextStream s(&m_logline.logstr);
            s << val;
        }

        return *this;
    }
};



#endif // CBCLOGGER_H















