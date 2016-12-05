#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // setup settings file, default values
    QCoreApplication::setOrganizationName("ZUT");
    QCoreApplication::setOrganizationDomain("zut.edu.pl");
    QCoreApplication::setApplicationName("BC Client");

    // set settings file name
    m_settingsPath = QCoreApplication::applicationDirPath() + "/settings.ini";
    mp_settings = new QSettings(m_settingsPath, QSettings::IniFormat);

    // check if settings file exists. If not, create it
    if (!fileExists(m_settingsPath))
        setDefaultSettings(mp_settings);

    // apply settings
    readAllSettings(mp_settings);

    // setup logger
    CBcLogger::instance()->startLogger("/tmp", true, MLL::ELogLevel::LDebug);

    // create socket object
    mp_tcpSocket = new QTcpSocket(this);

    // connect slots
    connect(mp_tcpSocket, SIGNAL(connected()),
            this, SLOT(on_tcpSocketConnected()));
    connect(mp_tcpSocket, SIGNAL(disconnected()),
            this, SLOT(on_tcpSocketDisconnected()));
    connect(mp_tcpSocket, SIGNAL(readyRead()),
            this, SLOT(on_tcpSocketReadyRead()));

    CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                 "Trying to connect to host %s:%u...", HOST_IP, HOST_PORT);

    mp_tcpSocket->connectToHost(HOST_IP, HOST_PORT);

    // check connection
    /*if (!mp_tcpSocket->waitForConnected(1000))
    {
        CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                                     "Unable to connect to host %s:%u, quiting", HOST_IP, HOST_PORT);
        QThread::usleep(500);
        exit(1);
    }*/

}

MainWindow::~MainWindow()
{
    delete ui;

    if (mp_tcpSocket)
        mp_tcpSocket->disconnect();

    if (mp_settings)
        mp_settings->deleteLater();
}

/*!
 * \brief MainWindow::fileExists: Checks either specified with \ref path file exists and is a file
 * \param path: path to the file
 * \return none zero if file exists
 */
bool MainWindow::fileExists(const QString& path)
{
    QFileInfo check_file(path);
    return (check_file.exists() && check_file.isFile());
}

/*!
 * \brief MainWindow::setDefaultSettings: Sets the default settings for the application
 * \param mp_settings: settings object pointer
 */
void MainWindow::setDefaultSettings(QSettings* mp_settings)
{
    Q_ASSERT(mp_settings);

    // network section
    mp_settings->beginGroup("network");
    mp_settings->setValue("ip", "10.10.10.1"); // default ip
    mp_settings->setValue("port", 12345); // default port
    mp_settings->endGroup();

    // window section
    mp_settings->beginGroup("window");

    // default window size
    QSize windowSize(1024, 600);
    mp_settings->setValue("size", windowSize);

    // default frame policy
    mp_settings->setValue("frame", 0);
    mp_settings->endGroup();

    // language section
    // add
}

/*!
 * \brief MainWindow::readAllSettings: Loads the settings from file
 * \param mp_settings
 */
void MainWindow::readAllSettings(QSettings* mp_settings)
{
    Q_ASSERT(mp_settings);

    // window section
    mp_settings->beginGroup("window");
    // window size:
    resize(mp_settings->value("size").value<QSize>());

    // frame policy
    if (0 == mp_settings->value("frame"))
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    mp_settings->endGroup();
}

void MainWindow::on_tcpSocketConnected()
{
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                 "Connected to host %s:%u...", HOST_IP, HOST_PORT);
}


void MainWindow::on_tcpSocketDisconnected()
{
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                 "Disconnected from host host %s:%u...", HOST_IP, HOST_PORT);
    QThread::usleep(500);
    exit(1);
}

void MainWindow::on_tcpSocketReadyRead()
{
    QByteArray rb = mp_tcpSocket->readAll();
    quint16 state = rb[0] | ((quint16)rb[1] << 8);

    ui->lLedState->setText(QString::number(state));
}













