#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QSettings>
#include <QFileInfo>

#include "cbclogger.h"
#include "cabstractmenu.h"
#include "cmainmenu.h"
#include "csettingsmenu.h"
#include "cdevicedialog.h"
#include "cbatteriesmenu.h"

#include "types.h"

#define HOST_IP     "10.10.10.1"
#define HOST_PORT   12345

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void on_tcpSocketConnected();
    void on_tcpSocketDisconnected();
    void on_tcpSocketReadyRead();

    void on_MenuBtnClicked(const EBtnTypes btn);

private slots:
    void on_tbMain_currentChanged(int index);

private:
    bool fileExists(const QString& path);
    void setDefaultSettings(QSettings* mp_settings);
    void readAllSettings(QSettings* mp_settings);
    CAbstractMenu* currentMenuObject(const int index);

    // members
    Ui::MainWindow *ui;

    QSettings*      mp_settings;        /*!< Settings file object */
    QString         m_settingsPath;     /*!< Setting file name with path */
    int             m_youngestTabIndex;

    // dialogs (menus)
    CMainMenu*      mp_mainMenu;
    CSettingsMenu*  mp_settingsMenu;
    CDeviceDialog*  mp_devicesMenu;
    CBatteriesMenu* mp_batMenu;

    QTcpSocket*     mp_tcpSocket;
};

#endif // MAINWINDOW_H
