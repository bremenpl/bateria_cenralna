#ifndef CBCLOGGER_H
#define CBCLOGGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QThread>

/*
 * @brief   Describes the log level for an item
 */
enum class ELogLevel
{
    eNone       = 0,
    e_Critical,
    e_Warning,
    e_Info,
    e_Debug
};

/*
 * @brief   Single log object descriptor
 */
struct logLine_t
{
    QString     logstr;         /*!< String to be saved to file */
    ELogLevel   loglvl;         /*!< Log level for an item */
    QDateTime   datetime;       /*!< Time stamp for the log item */
};

/*
 * @brief   Object is created in order to work in a separate thread. In this thread slot is called with queue
 *          in order to print the logs.
 */
class ClogPrinter : public QObject
{
    Q_OBJECT
public:
        using QObject::QObject;

signals:

public slots:
        void onLogEventHappened(const logLine_t& logline);
};

class CBcLogger : public QObject
{
    Q_OBJECT
public:
    // methods
    //explicit CBcLogger(QObject *parent = 0);
    static CBcLogger* instance();

    int startLogger(QString filename, bool verbose);
    void print(ELogLevel lvl, const char* text, ...);


signals:
    void addNewLogLine(const logLine_t& logline);

private:
// members
    static CBcLogger*   mp_instance;
    ELogLevel           m_setLogLvl;            /*!< Log level to compare */
    bool                m_loggerStarted;        /*!< logging possible only if set */
    bool                m_verbose;              /*!< Defines either logs should be also printed to console */
    QString             m_fileName;             /*!< Patch to the log file */
    QThread             m_printThread;          /*!< Printer object has to be moved to this thread */
    ClogPrinter         m_logPrinter;           /*!< Log printer object */

// methods
    CBcLogger();                              /*!< Hidden constructor cannot be called */


public slots:
};



#endif // CBCLOGGER_H















