#include "cmainmenu.h"
#include "ui_cmainmenu.h"
#include "mainwindow.h"

CMainMenu::CMainMenu(QWidget *parent) : CAbstractMenu(parent), ui(new Ui::CMainMenu)
{
    ui->setupUi(this);

    // set the menu name
    m_menuName = "Main Menu";

    // create new model
    mp_itemsModel = new QStandardItemModel(this);

    // add generic labels to the table view
    m_columnLabels << QObject::tr("Slave type") << QObject::tr("Amount");
    mp_itemsModel->setHorizontalHeaderLabels(m_columnLabels);

    // set the model
    ui->tvSlaves->setModel(mp_itemsModel);

    // resize columns
    for (int i = 0; i < mp_itemsModel->columnCount(); i++)
        ui->tvSlaves->setColumnWidth(i, 250);

    // add slave type rows
    addSlavesGroup(QStringLiteral("Total slaves"), 0);
    addSlavesGroup(QStringLiteral("Line controllers"), 0);
    addSlavesGroup(QStringLiteral("Relay controllers"), 0);

    // connect with main window
    auto mainObj = dynamic_cast<MainWindow*>(parent);

    connect(mainObj, SIGNAL(slavesChanged(const QVector<CBcSlaveDevice*>&)),
            this, SLOT(on_slavesChanged(const QVector<CBcSlaveDevice*>&)));
}

CMainMenu::~CMainMenu()
{
    delete ui;
}

void CMainMenu::addSlavesGroup(const QString& name, const int amount)
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

    // name
    mp_itemsModel->setData(mp_itemsModel->index(
                                 row, (int)itemsTableCol::slaveAddr, QModelIndex()), name, Qt::EditRole);

    // amount
    mp_itemsModel->setData(mp_itemsModel->index(
                                 row, (int)itemsTableCol::uniqId, QModelIndex()), amount, Qt::EditRole);
}

void CMainMenu::changeSlavesGroupAmount(const slaveRows slaves, const int amount)
{
    mp_itemsModel->setData(mp_itemsModel->index(
                               (int)slaves, 1, QModelIndex()), amount, Qt::EditRole);
}

void CMainMenu::on_slavesChanged(const QVector<CBcSlaveDevice*>& slaves)
{
    int slavesTotal = 0, slavesLc = 0, slavesRc = 0;

    foreach (CBcSlaveDevice* slave, slaves)
    {
        slavesTotal++;

        if (EDeviceTypes::LineCtrler == slave->slave_id().m_slaveType)
        {
            slavesLc++;

            for (int i = 0; i < slave->subSlaves().size(); i++)
            {
                if (EDeviceTypes::RelayCtrler == slave->subSlaves()[i]->slave_id().m_slaveType)
                {
                    slavesTotal++;
                    slavesRc++;
                }
            }
        }

        // TODO add batteries and etc counting
    }

    // update table
    changeSlavesGroupAmount(slaveRows::total, slavesTotal);
    changeSlavesGroupAmount(slaveRows::lcs, slavesLc);
    changeSlavesGroupAmount(slaveRows::rcs, slavesRc);
}

void CMainMenu::on_pbSettings_clicked()
{
    emit signal_BtnClicked(EBtnTypes::Settings);
}

void CMainMenu::on_pbDevices_clicked()
{
    emit signal_BtnClicked(EBtnTypes::Devices);
}
