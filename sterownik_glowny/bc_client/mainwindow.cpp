#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // clear unused pointers and assign zero values
    mp_tcpSocket = NULL;
    m_curLineCtrler = 0;
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

    // create socket object
    mp_tcpSocket = new QTcpSocket(this);

    // connect slots
    connect(mp_tcpSocket, SIGNAL(connected()),
            this, SLOT(on_tcpSocketConnected()));
    connect(mp_tcpSocket, SIGNAL(disconnected()),
            this, SLOT(on_tcpSocketDisconnected()));
    connect(mp_tcpSocket, SIGNAL(readyRead()),
            this, SLOT(on_tcpSocketReadyRead()), Qt::QueuedConnection);

    CBcLogger::instance()->print(MLL::ELogLevel::LInfo) <<
                                 QString("Trying to connect to host %1:%2...").arg(m_ip).arg(m_port);

    mp_tcpSocket->connectToHost(m_ip, m_port);

    // check connection
    if (!mp_tcpSocket->waitForConnected(1000))
    {
        CBcLogger::instance()->print(MLL::ELogLevel::LInfo) <<
                                     QString("Unable to connect to host %1:%2, quiting").arg(m_ip).arg(m_port);
        QThread::usleep(500);
        exit(1);
    }

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

    // network
    mp_settings->beginGroup("network");
    m_ip = mp_settings->value("ip").toString();
    m_port = mp_settings->value("port").toInt();
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
    CAbstractMenu* menuPanel = 0;
    QString name;

    switch (btn)
    {
        case EBtnTypes::Settings:
        {
            menuPanel = new CSettingsMenu(this);
            name = "Program\nsettings";
            break;
        }

        case EBtnTypes::Devices:
        {
            menuPanel = new CDeviceDialog(this);
            name = "Devices";
            break;
        }

        case EBtnTypes::LineControllers:
        {
            menuPanel = new CItemsLcMenu(this);
            name = "Line\nCtrls";
            break;
        }

        case EBtnTypes::RelayControllers:
        {
            menuPanel = new CItemsRcMenu(this);
            name = "Relay\nCtrls";
            break;
        }

        case EBtnTypes::PowerAdapter:
        {
            menuPanel = new CPaDevice(this);
            name = "Power\nadapter";
            break;
        }

        case EBtnTypes::Batteries:
        {
            menuPanel = new CItemsBatMenu(this);
            name = "Batt's";
            break;
        }

        case EBtnTypes::Chargers:
        {
            menuPanel = new CItemsCharMenu(this);
            name = "Char's";
            break;
        }

        default:
        {
             CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                     << "Unknown submenu button clicked with number " << (quint32)btn;
        }
    }

    // add object to tab widget
    if (menuPanel)
    {
        ui->tbMain->addTab(menuPanel, name);
        ui->tbMain->setCurrentIndex((ui->tbMain->indexOf(menuPanel)));
    }
}

/*!
 * \brief MainWindow::on_DeviceSelected: Slot run when user selects a device in on of the children forms
 * \param deviceType: Type of selected modbus device
 * \param slaveAddr: modbus slave address
 */
void MainWindow::on_DeviceSelected(const EDeviceTypes deviceType, const int slaveAddr)
{
    // save slave addr for later use
    m_curLineCtrler = slaveAddr;
    QString name;
    CAbstractMenu* menuPanel = 0;

    switch (deviceType)
    {
        case EDeviceTypes::LineCtrler:
        {
            name = "LC";
            menuPanel = new CPreLcPanel(this);
            break;
        }

        case EDeviceTypes::RelayCtrler:
        {
            name = "RC";
            menuPanel = new CRcDevice(this);
            break;
        }

        case EDeviceTypes::Battery:
        {
            name = "BAT";
            menuPanel = new CBatDevice(this);
            break;
        }

        case EDeviceTypes::Charger:
        {
            name = "CHAR";
            menuPanel = new CCharDevice(this);
            break;
        }

        default:
        {
            CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                    << "Unknown device selected: " << (quint32)deviceType;
        }
    }

    // add object to tab widget
    if (menuPanel)
    {
        name += " " + QString::number(m_curLineCtrler);
        ui->tbMain->addTab(menuPanel, name);
        ui->tbMain->setCurrentIndex((ui->tbMain->indexOf(menuPanel)));
    }

    CBcLogger::instance()->print(MLL::ELogLevel::LDebug,
                                 "Device of type %u with addr %u selected", (int)deviceType, slaveAddr);
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

void MainWindow::digForTcpFrames(const QByteArray& data)
{
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "New data from server %i", data.size());

    foreach (quint8 byte, data)
    {
        switch (m_tcpRxState)
        {
            case tcpRespState::devType:
            {
                mp_rxTcpFrame = new tcpFrame;
                mp_rxTcpFrame->data.clear();
                mp_rxTcpFrame->dType = (EDeviceTypes)byte;
                m_tcpRxState = tcpRespState::slaveAddr;
                break;
            }

            case tcpRespState::slaveAddr:
            {
                mp_rxTcpFrame->slaveAddr = byte;
                m_tcpRxState = tcpRespState::req;
                break;
            }

            case tcpRespState::req:
            {
                mp_rxTcpFrame->req = (tcpReq)byte;
                m_tcpRxState = tcpRespState::cmd;
                break;
            }

            case tcpRespState::cmd:
            {
                mp_rxTcpFrame->cmd = (tcpCmd)byte;
                m_tcpRxState = tcpRespState::len;
                break;
            }

            case tcpRespState::len:
            {
                mp_rxTcpFrame->len = (int)byte;
                m_tcpRxState = tcpRespState::data;
                break;
            }

            case tcpRespState::data:
            {
                mp_rxTcpFrame->data.append(byte);

                if (mp_rxTcpFrame->data.size() >= mp_rxTcpFrame->len)
                {
                    // frame fully parsed
                    m_rxTcpQueue.enqueue(mp_rxTcpFrame);

                    // set initial state
                    mp_rxTcpFrame = 0;
                    m_tcpRxState = tcpRespState::devType;
                }
                break;
            }

            default:
            {
                CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                        << "Unhandled tcpRxState: " << (int)m_tcpRxState;

                // initial state
                m_tcpRxState = tcpRespState::devType;

                if (mp_rxTcpFrame)
                {
                    delete mp_rxTcpFrame;
                    mp_rxTcpFrame = 0;
                }
            }
        }
    }

    // manage the frames if any new
    handleTcpRxFrames();
}

void MainWindow::handleTcpRxFrames()
{
    while (m_rxTcpQueue.size())
    {
        tcpFrame* frame = m_rxTcpQueue.dequeue();

        switch (frame->cmd)
        {
            case tcpCmd::presenceChanged:
            {
                bool presence = (bool)frame->data[0];
                QVector<slaveId*> pv; // parent vector
                int depthLevel = (frame->len - 1) / 2;

                for (int i = 0; i < depthLevel; i++)
                {
                    slaveId* slv =  new slaveId;
                    slv->m_slaveAddr = frame->data[1 + i * 2];
                    slv->m_slaveType = (EDeviceTypes)((quint8)frame->data[2 + i * 2]);
                    pv.append(slv);
                }

                if (presence)// add to slaves list
                    slavePresent(pv);

                break; // figure out how to remove slaves
            }

            default:
            {
                CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                        << "Unhandled tcp Rx cmd: " << (int)frame->cmd;
            }
        }

        // finally delete this frame
        delete frame;
    }
}

void MainWindow::slavePresent(QVector<slaveId*>& pv)
{
    // do nothing for empty cector
    if (!pv.size())
        return;

    CBcSlaveDevice* parentSlave = 0;
    QVector<CBcSlaveDevice*>* slaveVector = &m_slaves;

    for (int i = 0; i < pv.size(); i++)
    {
        if (parentSlave) // not base level
            slaveVector = &parentSlave->subSlaves();

        if (!slaveVector->size()) // no slaves yet, just append it
            appenSlave(pv[i], *slaveVector);
        else // at least one base level slave exists already
        {
            // check either the new slave is in the list already
            if (!slaveExists(pv[i], *slaveVector))
                appenSlave(pv[i], *slaveVector);
        }

        // update parent slave
        parentSlave = m_slaves.last();
    }
}

bool MainWindow::slaveExists(const slaveId* const slv, QVector<CBcSlaveDevice*>& slaveVector)
{
    Q_ASSERT(slv);

    foreach (CBcSlaveDevice* baseSlave, slaveVector)
    {
        if (baseSlave->slave_id().m_slaveAddr == slv->m_slaveAddr) // address match
            return true;
    }

    return false;
}

bool MainWindow::appenSlave(const slaveId* const slv, QVector<CBcSlaveDevice*>& slaveVector)
{
    Q_ASSERT(slv);
    CBcSlaveDevice* slave = 0;

    switch (slv->m_slaveType)
    {
        case EDeviceTypes::LineCtrler:  slave = new CBcLc(slv->m_slaveAddr); break;
        case EDeviceTypes::RelayCtrler: slave = new CBcRc(slv->m_slaveAddr); break;
        default:
        {
            CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                    << "Unhandled slave type: " << (int)slv->m_slaveType;
        }
    }

    if (slave) // append the base level slave
        slaveVector.append(slave);
    else
    {
        CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
            "Unable to append base lvl slave (ID: %u, TYPE: %i", slv->m_slaveAddr, (int)slv->m_slaveType);
    }

    return (bool)slave;
}


void MainWindow::on_tcpSocketConnected()
{
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo) << "Connected to server";
}

void MainWindow::on_tcpSocketDisconnected()
{
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo) << "Disconnected from server";

    // close for now
    close();
}

void MainWindow::on_tcpSocketReadyRead()
{
    digForTcpFrames(mp_tcpSocket->readAll());
}

























