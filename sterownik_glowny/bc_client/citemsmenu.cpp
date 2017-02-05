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

    m_columnLabels << QObject::tr("Slave address") << QObject::tr("Serial number") << "User string";
    mp_itemsModel->setHorizontalHeaderLabels(m_columnLabels);

    // set the model
    ui->tvItems->setModel(mp_itemsModel);

    connect(ui->tvItems->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(on_itemsSelectionChanged(const QItemSelection&, const QItemSelection&)));

    // connect with main window
    auto mainObj = dynamic_cast<MainWindow*>(parent);
    connect(this, SIGNAL(signal_deviceSelected(const EDeviceTypes, const int)),
            mainObj, SLOT(on_DeviceSelected(const EDeviceTypes, const int)));

    // resize columns
    for (int i = 0; i < mp_itemsModel->columnCount(); i++)
        ui->tvItems->setColumnWidth(i, 200);

    // TODO temp add some slaves
    for (int i = 0; i < 24; i++)
        addSlave(i + 1, QStringLiteral("unique ID"), QStringLiteral("user string"));

    // select 1st item
    selectNextItem(true);

    // buttons
    manageControls();
}

CItemsMenu::~CItemsMenu()
{
    delete ui;
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
    int row = selectedRow();

    if (row >= 0)
        emit signal_deviceSelected(m_deviceType, row + 1);
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













