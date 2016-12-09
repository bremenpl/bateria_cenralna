#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // clear unused pointers and assign zero values
    mp_settingsMenu = NULL;
    mp_devicesMenu = NULL;
    mp_batMenu = NULL;
    mp_tcpSocket = NULL;
    m_youngestTabIndex = -1;
    m_virtKeyboardOn = true;

    // setup logger
    CBcLogger::instance()->startLogger("/tmp", true, MLL::ELogLevel::LDebug);

    // set ui
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

    // set table view parameters
    ui->tbMain->setStyleSheet("QTabBar::tab { height: 100px; width: 100px; }");

    // create main submenu
    mp_mainMenu = new CMainMenu(this);
    ui->tbMain->addTab(mp_mainMenu, "Main\nmenu");

    /*// create socket object
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
    if (!mp_tcpSocket->waitForConnected(1000))
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

    // misc section
    mp_settings->beginGroup("misc");
    mp_settings->setValue("vkeyboard", 1); // virtual keyboard on
    mp_settings->endGroup();
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

    // keyboard
    mp_settings->beginGroup("misc");
    if (0 == mp_settings->value("vkeyboard"))
        m_virtKeyboardOn = false;
    mp_settings->endGroup();
}

/*!
 * \brief MainWindow::currentMenuObject: Gets the pointer under the tab index
 * \param index: index of the tab
 * \return pointer to the object under index
 */
CAbstractMenu* MainWindow::currentMenuObject(const int index)
{
    // get the object
    auto tabObject = dynamic_cast<CAbstractMenu*>(ui->tbMain->widget(index));
    return tabObject;
}

/*!
 * \brief MainWindow::on_MenuBtnClicked: Handles buttons from submenus.
 * \param btn: indicates which button on the submenu was clicked
 */
void MainWindow::on_MenuBtnClicked(const EBtnTypes btn)
{
    switch (btn)
    {
        case EBtnTypes::Settings:
        {
            mp_settingsMenu = new CSettingsMenu(this);
            ui->tbMain->addTab(mp_settingsMenu, "Program\nsettings");
            ui->tbMain->setCurrentIndex((ui->tbMain->indexOf(mp_settingsMenu)));
            break;
        }

        case EBtnTypes::Devices:
        {
            mp_devicesMenu = new CDeviceDialog(this);
            ui->tbMain->addTab(mp_devicesMenu, "Devices");
            ui->tbMain->setCurrentIndex((ui->tbMain->indexOf(mp_devicesMenu)));
            break;
        }

        case EBtnTypes::Batteries:
        {
            mp_batMenu = new CBatteriesMenu(this);
            ui->tbMain->addTab(mp_batMenu, "Battery");
            ui->tbMain->setCurrentIndex((ui->tbMain->indexOf(mp_batMenu)));
            break;
        }

        default:
        {
             CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                     << "Unknown submenu button clicked with number " << (quint32)btn;
        }
    }
}

/*!
 * \brief MainWindow::on_tbMain_currentChanged: Slot run when tab index is changed
 * \param index
 */
void MainWindow::on_tbMain_currentChanged(int index)
{
    // check if this is the youngest index
    if (index < m_youngestTabIndex)
    {
        // this is not the youngest index.
        // This index becomes youngest and all younger than him have to be destroyed.
        int target = m_youngestTabIndex - (m_youngestTabIndex - index);

        for (int i = m_youngestTabIndex; i > target; i--)
        {
            auto menuObj = currentMenuObject(i);
            ui->tbMain->removeTab(i);

            // delete the object from removed tab
            delete menuObj;
        }
    }

    // update index
    m_youngestTabIndex = index;
    ui->tbMain->setCurrentIndex(index);

    // log info about opened window
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo)
            << currentMenuObject(index)->menuName() << " opened";
}

void MainWindow::on_tcpSocketConnected()
{

}

void MainWindow::on_tcpSocketDisconnected()
{

}

void MainWindow::on_tcpSocketReadyRead()
{

}


