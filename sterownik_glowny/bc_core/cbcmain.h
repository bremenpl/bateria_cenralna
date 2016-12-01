#ifndef CBCMAIN_H
#define CBCMAIN_H

#include <QObject>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QString>
#include <QDebug>
#include <QDir>

#include "literals.h"
#include "cbclogger.h"
#include "cbcserialthread.h"
#include "cbctcpserver.h"

class CBcMain : public QObject
{
    Q_OBJECT
public:
    CBcMain(QCoreApplication& coreApp, QObject *parent = 0);
    ~CBcMain();

signals:

public slots:

private:
    // methods
    int getParameters(QCoreApplication& coreApp,
                      bool& verbose,
                      QString& logPath,
                      MLL::ELogLevel& logLevel,
                      QString& serial);

    // members
    bool                m_verbose;          /*!< print to console flag */
    QString             m_path;             /*!< log file path */
    MLL::ELogLevel      m_ll;               /*!< Maximum printable log level for a log msg */
    QString             m_serialName;       /*!< ModBus RTU master serial device name */
    CBcSerialThread*    mp_serialThread;    /*!< Serial thread used for handling modbus */
    QCoreApplication*   mp_coreApp;         /*!< Pointer to the core application object */

    CBcTcpServer        m_tcpServer;        /*!< TCP server responsilble for gui clients communication */
};

#endif // CBCMAIN_H
