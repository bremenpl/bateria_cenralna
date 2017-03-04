#include "citemsrcmenu.h"

CItemsRcMenu::CItemsRcMenu(const quint8 parentAddr, QWidget *parent) :
    CItemsMenu(parentAddr, parent)
{
    // set the menu name, it has to be replaced anyways
    m_menuName = "Relay Controllers Menu";

    // set device type
    m_deviceType = EDeviceTypes::RelayCtrler;
}

void CItemsRcMenu::slavesChangedDevSpecific(const QVector<CBcSlaveDevice*>& slaves)
{
    foreach (CBcSlaveDevice* slave, slaves)
    {
        if (slave->slave_id().m_slaveAddr == m_parentAddr)
        {
            foreach (CBcSlaveDevice* subslave, slave->subSlaves())
            {
                // add slave only if its proper type and is present
                if (m_deviceType == subslave->slave_id().m_slaveType)
                {
                    addSlave(subslave->slave_id().m_slaveAddr,
                             getUniqIdString(subslave->uniqId()),
                             QStringLiteral("user string"));
                }
            }
        }
    }
}

QVector<slaveId*> CItemsRcMenu::getParentVector(const int row)
{
    // Relay controller specific function, depth level = 2
    QVector<slaveId*> pv;

    if (row < 0)
        return pv;

    // parent
    slaveId* slv = new slaveId;
    slv->m_slaveType = EDeviceTypes::LineCtrler;
    slv->m_slaveAddr = m_parentAddr;
    pv.append(slv);

    // RC
    slv = new slaveId;
    slv->m_slaveType = EDeviceTypes::RelayCtrler;
    slv->m_slaveAddr = mp_itemsModel->data(
                mp_itemsModel->index(row, (int)itemsTableCol::slaveAddr, QModelIndex())).toUInt();
    pv.append(slv);

    return pv;
}









