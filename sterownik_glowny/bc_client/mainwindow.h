#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QSettings>
#include <QFileInfo>

#include "cbclogger.h"

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

private:
    bool fileExists(const QString& path);
    void setDefaultSettings(QSettings* mp_settings);
    void readAllSettings(QSettings* mp_settings);

    // members
    Ui::MainWindow *ui;

    QSettings*      mp_settings;        /*!< Settings file object */
    QString         m_settingsPath;     /*!< Setting file name with path */

    QTcpSocket*     mp_tcpSocket;
};

#endif // MAINWINDOW_H
