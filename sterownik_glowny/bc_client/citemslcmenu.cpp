#include "citemslcmenu.h"

CItemsLcMenu::CItemsLcMenu(const quint8 parentAddr, QWidget *parent) :
    CItemsMenu(parentAddr, parent)
{
    // set the menu name
    m_menuName = "Line Controllers Menu";

    // set device type
    m_deviceType = EDeviceTypes::LineCtrler;
}

void CItemsLcMenu::slavesChangedDevSpecific(const QVector<CBcSlaveDevice*>& slaves)
{
    // add top level slaves
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
}

QVector<slaveId*> CItemsLcMenu::getParentVector(const int row)
{
    // Line controller specific function, depth level = 1
    QVector<slaveId*> pv;

    if (row < 0)
        return pv;

    slaveId* slv = new slaveId;
    slv->m_slaveAddr = mp_itemsModel->data(
                mp_itemsModel->index(row, (int)itemsTableCol::slaveAddr, QModelIndex())).toUInt();
    slv->m_slaveType = EDeviceTypes::LineCtrler;
    pv.append(slv);

    return pv;
}




