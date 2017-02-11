#include "citemsmenu.h"
#include "ui_citemsmenu.h"
#include "mainwindow.h"

CItemsMenu::CItemsMenu(QWidget *parent) : CAbstractMenu(parent), ui(new Ui::CItemsMenu)
{
    // start values
    m_noOfItems = 0;
    m_columnLabels.clear();

    ui->setupUi(this);

    // set the menu name, it has to be replaced anyways
    m_menuName = "Items Menu";

    // create new model
    mp_itemsModel = new QStandardItemModel(this);

    // add generic labels to the table view
    m_columnLabels << QObject::tr("Slave address")
                   << QObject::tr("Serial number")
                   << QObject::tr("User string");
    mp_itemsModel->setHorizontalHeaderLabels(m_columnLabels);

    // set the model
    ui->tvItems->setModel(mp_itemsModel);

    connect(ui->tvItems->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(on_itemsSelectionChanged(const QItemSelection&, const QItemSelection&)));

    // connect with main window
    auto mainObj = dynamic_cast<MainWindow*>(parent);

    connect(this, SIGNAL(deviceSelected(const EDeviceTypes, const int)),
            mainObj, SLOT(on_DeviceSelected(const EDeviceTypes, const int)));

    connect(mainObj, SIGNAL(slavesChanged(const QVector<CBcSlaveDevice*>&)),
            this, SLOT(on_slavesChanged(const QVector<CBcSlaveDevice*>&)), Qt::QueuedConnection);

    // resize columns
    for (int i = 0; i < mp_itemsModel->columnCount(); i++)
        ui->tvItems->setColumnWidth(i, 200);

    // buttons
    manageControls();
}

CItemsMenu::~CItemsMenu()
{
    delete ui;
}

QString CItemsMenu::getUniqIdString(const quint16* const uniqId)
{
    Q_ASSERT(uniqId);
    QString uniqIdStr = "0x";

    for (int i = UNIQ_ID_REGS - 1; i >= 0; i--)
        uniqIdStr += QString("%1").arg(uniqId[i], 2, 16, QChar('0'));

    return uniqIdStr;
}

void CItemsMenu::addSlave(const quint8 addr, const QString& uniqId, const QString& userString)
{
    // create new items
    QList<QStandardItem*> itemsList;
    for (int i = 0; i < m_columnLabels.size(); i++)
    {
         itemsList.append(new QStandardItem(1));
         itemsList.last()->setEditable(false);
    }

    // append new item
    mp_itemsModel->appendRow(itemsList);
    int row = mp_itemsModel->rowCount() - 1;

    // slave address
    mp_itemsModel->setData(mp_itemsModel->index(
                                 row, (int)itemsTableCol::slaveAddr, QModelIndex()), addr, Qt::EditRole);

    // unique ID
    mp_itemsModel->setData(mp_itemsModel->index(
                                 row, (int)itemsTableCol::uniqId, QModelIndex()), uniqId, Qt::EditRole);

    // user string
    mp_itemsModel->setData(mp_itemsModel->index(
                                 row, (int)itemsTableCol::userStr, QModelIndex()), userString, Qt::EditRole);
}

int CItemsMenu::selectedRow()
{
    Q_ASSERT(mp_itemsModel);

    if (!mp_itemsModel->rowCount()) // no items to select
        return -1;
    else if (ui->tvItems->selectionModel()->selectedRows().size() > 1) // more than 1 item selected
        return -2;
    else if (!ui->tvItems->selectionModel()->hasSelection()) // no selection
        return -3;
    else
    {
        foreach (auto index, ui->tvItems->selectionModel()->selectedRows())
            return index.row();
    }

    return -4; // wont get here
}

void CItemsMenu::selectNextItem(const bool nextItem)
{
    int row = selectedRow();

    // no items in table or too many selected
    if ((row == -1) || (row == -2))
        return;


    // no selected items but they exist in the table
    else if (row == -3)
    {
        row = 0;
        ui->tvItems->selectRow(row);
    }

    else
    {
        // one selected item
        foreach (auto index, ui->tvItems->selectionModel()->selectedRows())
        {
            if (nextItem)
            {
                row = index.row() + 1;

                // check if next row exists
                if (row > (mp_itemsModel->rowCount() - 1))
                    row = 0;
            }
            else
            {
                row = index.row() - 1;

                // check if prev row exists
                if (row <= 0)
                    row = mp_itemsModel->rowCount() - 1;
            }

            ui->tvItems->selectRow(row);
        }
    }
}

void CItemsMenu::manageControls()
{
    bool hasSelection = ui->tvItems->selectionModel()->hasSelection();

    ui->pbItemsDown->setEnabled(hasSelection);
    ui->pbItemsUp->setEnabled(hasSelection);
    ui->pbItemsEnter->setEnabled(hasSelection);
}

void CItemsMenu::on_slavesChanged(const QVector<CBcSlaveDevice*>& slaves)
{
    (void)slaves;
    CBcLogger::instance()->print(MLL::ELogLevel::LDebug)
            << "For " << m_menuName << " slaves state changed";

    // remove all slaves from the table
    if (mp_itemsModel->rowCount())
        mp_itemsModel->removeRows(0, mp_itemsModel->rowCount(), QModelIndex());

    // add slaves
    foreach (CBcSlaveDevice* slave, slaves)
    {
        // add slave only if its proper type and is present
        if (m_deviceType == slave->slave_id().m_slaveType)
        {
            addSlave(slave->slave_id().m_slaveAddr,
                     getUniqIdString(slave->uniqId()),
                     QStringLiteral("user string"));
        }
    }

    // let children decide what to do
    slavesChangedDevSpecific(slaves);

    // select 1st item
    selectNextItem(true);
}

void CItemsMenu::slavesChangedDevSpecific(const QVector<CBcSlaveDevice*>& slaves)
{
    (void)slaves;
    CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
            << "slavesChangedDevSpecific run in base class!";
}

void CItemsMenu::on_pbItemsUp_clicked()
{
    selectNextItem(false);
}

void CItemsMenu::on_pbItemsDown_clicked()
{
    selectNextItem(true);
}

void CItemsMenu::on_pbItemsEnter_clicked()
{
    // a list of selected rows is needed (will be 1)
    foreach (auto ind, ui->tvItems->selectionModel()->selectedRows())
    {
        quint8 slaveId = mp_itemsModel->data(mp_itemsModel->index(ind.row(), 0, QModelIndex())).toUInt();

        if(slaveId)
            emit deviceSelected(m_deviceType, slaveId);
        break; // to make sure there is just one
    }
}

void CItemsMenu::on_itemsSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    (void)deselected;
    (void)selected;

    foreach (auto index, ui->tvItems->selectionModel()->selectedRows())
        CBcLogger::instance()->print(MLL::ELogLevel::LDebug,
            "Items type %i row selected %i", (int)m_deviceType, index.row());

    manageControls();
}













