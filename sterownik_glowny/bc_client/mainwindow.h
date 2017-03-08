#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QSettings>
#include <QFileInfo>
#include <QQueue>
#include <QVector>

#include "cbclogger.h"
#include "cabstractmenu.h"
#include "cmainmenu.h"
#include "csettingsmenu.h"
#include "cdevicedialog.h"
#include "cbatteriesmenu.h"
#include "citemslcmenu.h"
#include "cprelcpanel.h"
#include "citemsrcmenu.h"
#include "crcdevice.h"
#include "cpadevice.h"
#include "citemsbatmenu.h"
#include "cbatdevice.h"
#include "citemscharmenu.h"
#include "cchardevice.h"
#include "types.h"
#include "ctcpparser.h"

// from core
#include "cbclc.h"
#include "cbcrc.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool virtKeyboardOn() { return m_virtKeyboardOn; }
    QVector<CBcSlaveDevice*>* slaves() { return &m_slaves; }
    QVector<slaveId*>* selectedPv() { return &m_pv; }

public slots:
    void on_tcpSocketConnected();
    void on_tcpSocketDisconnected();
    void on_tcpSocketReadyRead();

    void on_MenuBtnClicked(const EBtnTypes btn);
    void on_DeviceSelected(const EDeviceTypes deviceType, const int slaveAddr);
    void on_getRcStatusReg(const QVector<slaveId*>* pv);
    void on_setRcRelayState(bool state, const QVector<slaveId*>* pv);

signals:
    void slavesChanged(const QVector<CBcSlaveDevice*>& slaves);

private slots:
    void on_tbMain_currentChanged(int index);
    void on_slavesChanged(const QVector<CBcSlaveDevice*>& slaves);

    void on_getSlaveUniqId(const QVector<slaveId*>& pv);
    void on_newFramesAvailable(QQueue<tcpFrame*>* framesQueue);

private:
    bool fileExists(const QString& path);
    void setDefaultSettings(QSettings* mp_settings);
    void readAllSettings(QSettings* mp_settings);
    CAbstractMenu* currentMenuObject(const int index);

    void slavePresent(QVector<slaveId*>& pv);
    void slaveAbsent(QVector<slaveId*>& pv);
    bool appendSlave(const slaveId* const slv, QVector<CBcSlaveDevice*>& slaveVector);
    bool removeSlave(const slaveId* const slv, QVector<CBcSlaveDevice*>& slaveVector);
    QVector<CBcSlaveDevice*>* getSlaveIndex(QVector<slaveId*>& pv, int& index);

    void slaveUniqIdObtained(const quint16* uniqId, QVector<slaveId*>& pv);
    void slaveStatusRegObtained(const quint16 status, QVector<slaveId*>& pv);
    void slaveRcBitChanged(const bool state, QVector<slaveId*>& pv);
    void sendDataRequest(const tcpFrame& frame);
    void sendData2Socket(const QByteArray& data);

    // members
    Ui::MainWindow *ui;

    QSettings*      mp_settings;        /*!< Settings file object */
    QString         m_settingsPath;     /*!< Setting file name with path */
    int             m_youngestTabIndex;
    bool            m_virtKeyboardOn;

    // dialogs (menus)
    CMainMenu*      mp_mainMenu;

    QTcpSocket*     mp_tcpSocket;
    QString         m_ip;
    int             m_port;
    CTcpParser      m_tcpParser;

    QVector<CBcSlaveDevice*> m_slaves;      /*!< "list" of slave devices */
    slaveId         m_selectedSlave;
    slaveId         m_selectedSubSlave;     /*!< Maximum of 1 subslave for now */
    QVector<slaveId*> m_pv;                 /*!< Parent vector */
};

#endif // MAINWINDOW_H
