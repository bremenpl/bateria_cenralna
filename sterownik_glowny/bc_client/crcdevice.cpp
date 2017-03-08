#include "crcdevice.h"
#include "mainwindow.h"


CRcDevice::CRcDevice(QWidget *parent) : CAbstractDevice(parent)
{
    // set the menu name
    m_menuName = "Relay controller device";

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
                                 row, (int)paramsTableCol::name, QModelIndex()), "Relay", Qt::EditRole);

    // access
    mp_itemsModel->setData(mp_itemsModel->index(
                                 row, (int)paramsTableCol::access, QModelIndex()), "RW", Qt::EditRole);

    // value
    mp_relayBtn = new QPushButton(this);
    mp_relayBtn->setText("Unknown");
    mp_tv->setIndexWidget(mp_itemsModel->index(0, (int)paramsTableCol::value), mp_relayBtn);

    // connect button presses
    connect(mp_relayBtn, &QPushButton::clicked,
            this, &CRcDevice::on_relayBtnPressed);

    itemsList.clear();
    for (int i = 0; i < m_columnLabels.size(); i++)
    {
        itemsList.append(new QStandardItem(1));
        itemsList.last()->setEditable(false);
    }

    // append new item
    mp_itemsModel->appendRow(itemsList);
    row = mp_itemsModel->rowCount() - 1;

    // name
    mp_itemsModel->setData(mp_itemsModel->index(
                                 row, (int)paramsTableCol::name, QModelIndex()), "CurDet", Qt::EditRole);

    // access
    mp_itemsModel->setData(mp_itemsModel->index(
                                 row, (int)paramsTableCol::access, QModelIndex()), "R", Qt::EditRole);

    // value
    mp_itemsModel->setData(mp_itemsModel->index(
                                 row, (int)paramsTableCol::value, QModelIndex()), "Unknown", Qt::EditRole);

    // connect with main window
    auto mainObj = dynamic_cast<MainWindow*>(parent);
    Q_ASSERT(mainObj);
    connect(this, &CRcDevice::getRcStatusReg,
            mainObj, &MainWindow::on_getRcStatusReg, Qt::QueuedConnection);
    connect(this, &CRcDevice::setRcRelayState,
            mainObj, &MainWindow::on_setRcRelayState, Qt::QueuedConnection);

    /*// 1st parameter: Relay status
    QTreeWidgetItem* item = new QTreeWidgetItem(mp_tw);
    item->setText(0, "Relay status");
    item->setText(1, "RW");
    item->setText(2, "OFF");

    // 2nd parameter: Power source (power grid or battery)
    item = new QTreeWidgetItem(mp_tw);
    item->setText(0, "Power source");
    item->setText(1, "R");
    item->setText(2, "Power grid");

    // 3rd parameter: blackout behaviour
    item = new QTreeWidgetItem(mp_tw);
    item->setText(0, "Blackout action");
    item->setText(1, "RW");
    item->setText(2, "Switch to bat");*/

}

void CRcDevice::on_relayBtnPressed(bool checked)
{
    (void)checked;
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo) << QStringLiteral("Relay button pressed");

    if (mp_relayBtn->text() == "Unknown")
        return;
    else if (mp_relayBtn->text() == "OFF")
        emit setRcRelayState(true, mp_pv);
    else // ON
        emit setRcRelayState(false, mp_pv);
}

void CRcDevice::rowSelected(const int row)
{
    switch ((paramTableRow)row)
    {
        case paramTableRow::RS:
        {
            if (mp_relayBtn->text() == "Unknown")
                emit getRcStatusReg(mp_pv);
            break;
        }

        case paramTableRow::CD:
        {
            emit getRcStatusReg(mp_pv);
            break;
        }

        default:
        {
            CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                    << "Unhandled row " << row << "selected in rcdevice class";
        }
    }
}

void CRcDevice::on_slavesChanged(const QVector<CBcSlaveDevice*>& slaves)
{
    if (!slaves.size())
        return;

    foreach (CBcSlaveDevice* slave, slaves)
    {
        // find LC
        if (slave->slave_id().m_slaveAddr == mp_pv->first()->m_slaveAddr)
        {
            foreach (CBcSlaveDevice* subSlave, slave->subSlaves())
            {
                // find RC at second depth level
                if (subSlave->slave_id().m_slaveAddr == mp_pv->at(1)->m_slaveAddr)
                {
                    //CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "LC %u RC %u",
                        //slave->slave_id().m_slaveAddr, subSlave->slave_id().m_slaveAddr);

                    // check status register
                    auto rc = dynamic_cast<CBcRc*>(subSlave);
                    if (!(rc->statusReg() & (1 << (int)statusRegBits::RS)))
                        mp_relayBtn->setText("OFF");
                    else
                        mp_relayBtn->setText("ON");

                    QString cd;
                    if (!(rc->statusReg() & (1 << (int)statusRegBits::CD)))
                        cd = "No current";
                    else
                        cd = "Current present";

                    mp_itemsModel->setData(mp_itemsModel->index(
                        (int)paramTableRow::CD, (int)paramsTableCol::value, QModelIndex()),
                        cd, Qt::EditRole);
                    break;
                }
            }
            break;
        }
    }
}



















