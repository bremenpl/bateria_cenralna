#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // clear unused pointers and assign zero values
    mp_tcpSocket = NULL;
    m_youngestTabIndex = -1;
    m_virtKeyboardOn = true;
    m_selectedSlave.m_slaveType = EDeviceTypes::Dummy;
    m_selectedSlave.m_slaveAddr = 0;

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
    connect(this, SIGNAL(slavesChanged(const QVector<CBcSlaveDevice*>&)),
            this, SLOT(on_slavesChanged(const QVector<CBcSlaveDevice*>&)));
    connect(&m_tcpParser, SIGNAL(newFramesAvailable(QQueue<tcpFrame*>*)),
            this, SLOT(on_newFramesAvailable(QQueue<tcpFrame*>*)));

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
            menuPanel = new CItemsLcMenu(0, this);
            name = "Line\nCtrls";
            break;
        }

        case EBtnTypes::RelayControllers:
        {
            menuPanel = new CItemsRcMenu(m_selectedSlave.m_slaveAddr, this);
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
            menuPanel = new CItemsBatMenu(0, this);
            name = "Batt's";
            break;
        }

        case EBtnTypes::Chargers:
        {
            menuPanel = new CItemsCharMenu(0, this);
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

    emit slavesChanged(m_slaves);
}

/*!
 * \brief MainWindow::on_DeviceSelected: Slot run when user selects a device in on of the children forms
 * \param deviceType: Type of selected modbus device
 * \param slaveAddr: modbus slave address
 */
void MainWindow::on_DeviceSelected(const EDeviceTypes deviceType, const int slaveAddr)
{
    QString name;
    CAbstractMenu* menuPanel = 0;

    switch (deviceType)
    {
        case EDeviceTypes::LineCtrler:
        {
            name = "LC";
            menuPanel = new CPreLcPanel(this);

            m_selectedSlave.m_slaveAddr = slaveAddr;
            m_selectedSlave.m_slaveType = deviceType;
            m_selectedSubSlave.m_slaveAddr = 0;
            m_selectedSubSlave.m_slaveType = EDeviceTypes::Dummy;
            break;
        }

        case EDeviceTypes::RelayCtrler:
        {
            name = "RC";
            menuPanel = new CRcDevice(this);

            m_selectedSubSlave.m_slaveAddr = slaveAddr;
            m_selectedSubSlave.m_slaveType = deviceType;
            break;
        }

        case EDeviceTypes::Battery:
        {
            name = "BAT";
            menuPanel = new CBatDevice(this);

            m_selectedSlave.m_slaveAddr = slaveAddr;
            m_selectedSlave.m_slaveType = deviceType;
            m_selectedSubSlave.m_slaveAddr = 0;
            m_selectedSubSlave.m_slaveType = EDeviceTypes::Dummy;
            break;
        }

        case EDeviceTypes::Charger:
        {
            name = "CHAR";
            menuPanel = new CCharDevice(this);

            m_selectedSlave.m_slaveAddr = slaveAddr;
            m_selectedSlave.m_slaveType = deviceType;
            m_selectedSubSlave.m_slaveAddr = 0;
            m_selectedSubSlave.m_slaveType = EDeviceTypes::Dummy;
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
        name += " " + QString::number(slaveAddr);
        ui->tbMain->addTab(menuPanel, name);
        ui->tbMain->setCurrentIndex((ui->tbMain->indexOf(menuPanel)));

        emit slavesChanged(m_slaves);
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

/*!
 * \brief MainWindow::on_slavesChanged: In this method it is neccesarry to find out
 * either the last selected slave is on the list. If its not, then menu related to it
 * has to be closed, as the slave is not present anymore.
 * \param slaves
 */
void MainWindow::on_slavesChanged(const QVector<CBcSlaveDevice*>& slaves)
{
    if (EDeviceTypes::Dummy == m_selectedSlave.m_slaveType)
        return;

    // this is dependent on slave type
    switch (m_selectedSlave.m_slaveType)
    {
        case EDeviceTypes::LineCtrler:
        {
            foreach (CBcSlaveDevice* slave, slaves)
            {
                if (slave->slave_id().m_slaveAddr == m_selectedSlave.m_slaveAddr)
                {
                    if (slave->slave_id().m_slaveType == m_selectedSlave.m_slaveType)
                        return; // device still there, quit
                }
            }

            // device not found, its absent
            // changing the index to top level device index will destroy further windows
            m_selectedSlave.m_slaveAddr = 0;
            m_selectedSlave.m_slaveType = EDeviceTypes::Dummy;
            m_selectedSubSlave.m_slaveAddr = 0;
            m_selectedSubSlave.m_slaveType = EDeviceTypes::Dummy;
            ui->tbMain->setCurrentIndex(2);
            break;
        }

        default:
        {
            CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                << "Unhandled slave changed in on_slavesChanged: "
                << (int)m_selectedSlave.m_slaveType;
        }
    }
}

void MainWindow::on_newFramesAvailable(QQueue<tcpFrame*>* framesQueue)
{
    Q_ASSERT(framesQueue);

    while (framesQueue->size())
    {
        tcpFrame* frame = framesQueue->dequeue();
        QVector<slaveId*> pv; // parent vector
        int depthLevel = 0, dataLen = 0;

        switch (frame->cmd)
        {
            case tcpCmd::presenceChanged:
            {
                bool presence = (bool)frame->data[0];
                dataLen = 1;
                depthLevel = (frame->len - dataLen) / 2;
                pv.append(CTcpParser::getParentVector(frame, depthLevel, dataLen));

                if (presence)
                    slavePresent(pv); // add to slaves list
                else
                    slaveAbsent(pv); // remove from slaves list
                break;
            }

            case tcpCmd::takeUniqId:
            {
                quint16 uniqId[UNIQ_ID_REGS];
                for (int i = 0, k = 0; i < UNIQ_ID_REGS; i++, k += 2)
                {
                    uniqId[i] = frame->data[k];
                    uniqId[i] |= (frame->data[k + 1] << 8);
                }

                dataLen = 12;
                depthLevel = (frame->len - dataLen) / 2;
                pv.append(CTcpParser::getParentVector(frame, depthLevel, dataLen));

                slaveUniqIdObtained(uniqId, pv);
                break;
            }

            default:
            {
                CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                        << "Unhandled tcp Rx cmd: " << (int)frame->cmd;
            }
        }

        // inform interested menus about the change
        if (depthLevel)
            emit slavesChanged(m_slaves);

        // finally delete this frame
        delete frame;
        pv.clear();
    }
}

void MainWindow::slavePresent(QVector<slaveId*>& pv)
{
    int index = 0;
    QVector<CBcSlaveDevice*>* sv = getSlaveIndex(pv, index);

    if (index < 0)
        appendSlave(pv.last(), *sv);
}

void MainWindow::slaveAbsent(QVector<slaveId*>& pv)
{
    int index = 0;
    QVector<CBcSlaveDevice*>* sv = getSlaveIndex(pv, index);

    if (index >= 0)
        removeSlave(&sv->at(index)->slave_id(), *sv);
}

bool MainWindow::appendSlave(const slaveId* const slv, QVector<CBcSlaveDevice*>& slaveVector)
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
    {
        CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "SlaveAddr %u Type %i present",
            slave->slave_id().m_slaveAddr, (int)slave->slave_id().m_slaveType);

        slaveVector.append(slave);
    }
    else
    {
        CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
            "Unable to append base lvl slave (ID: %u, TYPE: %i", slv->m_slaveAddr, (int)slv->m_slaveType);
    }

    return (bool)slave;
}

bool MainWindow::removeSlave(const slaveId* const slv, QVector<CBcSlaveDevice*>& slaveVector)
{
    Q_ASSERT(slv);

    for (int i = 0; i < slaveVector.size(); i++)
    {
        if (slaveVector[i]->slave_id().m_slaveAddr == slv->m_slaveAddr) // address match
        {
            CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                "SlaveAddr %u Type %i absent", slv->m_slaveAddr, (int)slv->m_slaveType);

            slaveVector.remove(i); // remove from vector and and free memory
            return true;
        }
    }

    return false;
}

static int findSlaveIndex(const slaveId* const slv, QVector<CBcSlaveDevice*>& slaveVector)
{
    Q_ASSERT(slv);

    for (int i = 0; i < slaveVector.size(); i++)
    {
        if (slaveVector[i]->slave_id().m_slaveAddr == slv->m_slaveAddr) // address match
        {
            if (slaveVector[i]->slave_id().m_slaveType == slv->m_slaveType) // type match
            return i;
        }
    }

    return -1;
}

QVector<CBcSlaveDevice*>* MainWindow::getSlaveIndex(QVector<slaveId*>& pv, int& index)
{
    if (!pv.size())
    {
        index = -1;
        return 0;
    }

    CBcSlaveDevice* parentSlave = 0;
    QVector<CBcSlaveDevice*>* slaveVector = &m_slaves;

    for (int i = 0; i < pv.size(); i++)
    {
        if (parentSlave) // not base level
            slaveVector = &parentSlave->subSlaves();

        // update parent slave
        if (pv.size() > (i + 1))
            parentSlave = slaveVector->last();
    }

    // save found values
    index = findSlaveIndex(pv.last(), *slaveVector);
    return slaveVector;
}

void MainWindow::slaveUniqIdObtained(const quint16* uniqId, QVector<slaveId*>& pv)
{
    int index = 0;
    QVector<CBcSlaveDevice*>* sv = getSlaveIndex(pv, index);

    if (index >= 0)
        sv->at(index)->setUniqId(uniqId);
}

/*!
 * \brief MainWindow::on_getSlaveUniqId: in this function request an unique ID from a modbus slave
 * \param pv
 */
void MainWindow::on_getSlaveUniqId(const QVector<slaveId*>& pv)
{
    if (!pv.size())
    {
        CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                << "on_getSlaveUniqId empty parent vector!";
        return;
    }

    tcpFrame frame;
    frame.dType = pv.last()->m_slaveType;
    frame.slaveAddr = pv.last()->m_slaveAddr;
    frame.req = tcpReq::get;
    frame.cmd = tcpCmd::takeUniqId;

    // append parent vector
    foreach (slaveId* item, pv)
    {
        frame.data.append(item->m_slaveAddr);
        frame.data.append((quint8)item->m_slaveType);
    }

    frame.len = frame.data.size();
    sendDataRequest(frame);
}

void MainWindow::sendDataRequest(const tcpFrame& frame)
{
    if (!mp_tcpSocket->waitForConnected(1))
    {
        CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                << "Not connected to server, cant send requests through TCP";
        return;
    }

    QByteArray rb;
    rb.append((quint8)frame.dType);     // device type
    rb.append(frame.slaveAddr);         // slave address
    rb.append((quint8)frame.req);       // request type
    rb.append((quint8)frame.cmd);       // command
    rb.append((quint8)frame.len);       // data len
    rb.append(frame.data);              // data

    sendData2Socket(rb);
}

void MainWindow::sendData2Socket(const QByteArray& data)
{
    int lenSent = mp_tcpSocket->write(data, data.size());

    if (lenSent)
        CBcLogger::instance()->print(MLL::ELogLevel::LDebug,
                                     "%u bytes sent to server", lenSent);
    else
        CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                                     "Failed to send data to server");
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
    m_tcpParser.digForTcpFrames(mp_tcpSocket->readAll());
}

























