#ifndef CBCMAIN_H
#define CBCMAIN_H

#include <QObject>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QString>
#include <QDebug>
#include <QDir>
#include <QSettings>

#include "literals.h"
#include "cbclogger.h"
#include "cbclcthread.h"
#include "cbctcpserver.h"

class CBcMain : public QObject
{
    Q_OBJECT
public:
    CBcMain(QCoreApplication& coreApp, QObject *parent = 0);
    ~CBcMain();

signals:
    void setPingParameters(const quint32 noOfPings, const quint32 noOfDevices);

public slots:

private:
    // methods
    int getOrCteareSettings(QCoreApplication& coreApp);
    void readSettings();

    // members
    bool                m_verbose;          /*!< print to console flag */
    QString             m_logPath;          /*!< log file path */
    MLL::ELogLevel      m_ll;               /*!< Maximum printable log level for a log msg */

    bool                m_serialsValid;     /*!< Serial ports validation flag */
    quint32             m_noOfPings;        /*!< Number of pings needed for presence */

    QString             m_lcSerialName;     /*!< ModBus RTU master serial device name */
    CBcLcThread*        mp_lcSerialThread;  /*!< Serial thread used for handling line controllers modbus */
    quint32             m_noOfLcSlaves;     /*!< Amount of line controllers to scan */
    quint32             m_noOfRcPerLc;      /*!< Amount of relay controllers per line controller */

    QString             m_settingsPath;     /*!< path to the settings file  */
    QSettings*          mp_settings;        /*!< Settings object */

    CBcTcpServer        m_tcpServer;        /*!< TCP server responsilble for gui clients communication */
};

#endif // CBCMAIN_H
