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

public slots:
    void on_tcpSocketConnected();
    void on_tcpSocketDisconnected();
    void on_tcpSocketReadyRead();

    void on_MenuBtnClicked(const EBtnTypes btn);
    void on_DeviceSelected(const EDeviceTypes deviceType, const int slaveAddr);

signals:
    void slavesChanged(const QVector<CBcSlaveDevice*>& slaves);

private slots:
    void on_tbMain_currentChanged(int index);
    void on_slavesChanged(const QVector<CBcSlaveDevice*>& slaves);

private:
    bool fileExists(const QString& path);
    void setDefaultSettings(QSettings* mp_settings);
    void readAllSettings(QSettings* mp_settings);
    CAbstractMenu* currentMenuObject(const int index);
    void digForTcpFrames(const QByteArray& data);
    void handleTcpRxFrames();

    void slavePresent(QVector<slaveId*>& pv);
    void slaveAbsent(QVector<slaveId*>& pv);
    bool appendSlave(const slaveId* const slv, QVector<CBcSlaveDevice*>& slaveVector);
    bool removeSlave(const slaveId* const slv, QVector<CBcSlaveDevice*>& slaveVector);
    QVector<CBcSlaveDevice*>* getSlaveIndex(QVector<slaveId*>& pv, int& index);

    void slaveUniqIdObtained(const quint16* uniqId, QVector<slaveId*>& pv);

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

    tcpRespState    m_tcpRxState = tcpRespState::devType;
    tcpFrame*       mp_rxTcpFrame = 0;
    QQueue<tcpFrame*> m_rxTcpQueue;

    QVector<CBcSlaveDevice*> m_slaves;      /*!< "list" of slave devices */
    slaveId         m_selectedSlave;
    slaveId         m_selectedSubSlave;     /*!< Maximum of 1 subslave for now */
};

#endif // MAINWINDOW_H
