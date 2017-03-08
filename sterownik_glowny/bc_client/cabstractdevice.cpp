#include "cabstractdevice.h"
#include "ui_cabstractdevice.h"
#include "mainwindow.h"

CAbstractDevice::CAbstractDevice(QWidget *parent) : CAbstractMenu(parent), ui(new Ui::CAbstractDevice)
{
    ui->setupUi(this);
    m_columnLabels.clear();

    // set the menu name, it has to be replaced anyways
    m_menuName = "Astract device menu";

    // create new model
    mp_itemsModel = new QStandardItemModel(this);

    // add generic labels to the table view
    m_columnLabels << QObject::tr("Parameter")
                   << QObject::tr("Access")
                   << QObject::tr("Value");

    mp_itemsModel->setHorizontalHeaderLabels(m_columnLabels);

    // set the model
    mp_tv = ui->tvParams;
    mp_tv->setModel(mp_itemsModel);

    // resize columns
    mp_tv->setColumnWidth((int)paramsTableCol::name, 150);
    mp_tv->setColumnWidth((int)paramsTableCol::access, 100);
    mp_tv->setColumnWidth((int)paramsTableCol::value, 250);

    // signals and slots
    connect(mp_tv->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CAbstractDevice::on_selectionChanged);

    // get parent vector
    auto mainObj = dynamic_cast<MainWindow*>(parent);
    mp_pv = mainObj->selectedPv();
    Q_ASSERT(mp_pv);

    for (int i = 0; i < mp_pv->size(); i++)
    {
        CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Device menu slave %u, depth %u",
            mp_pv->at(i)->m_slaveAddr, i);
    }

    connect(mainObj, &MainWindow::slavesChanged,
            this, &CAbstractDevice::on_slavesChanged, Qt::QueuedConnection);
}

void CAbstractDevice::on_slavesChanged(const QVector<CBcSlaveDevice*>& slaves)
{
    (void)slaves;

    CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
        "Abstract device slaves changed slot");
}

CAbstractDevice::~CAbstractDevice()
{
    delete ui;
}

void CAbstractDevice::on_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    (void)deselected;
    (void)selected;

    foreach (auto index, mp_tv->selectionModel()->selectedRows())
    {
        rowSelected(index.row());
        break;
    }
}

void CAbstractDevice::rowSelected(const int row)
{
    (void)row;

    CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
            << "rowSelected method called in abstract class";
}
