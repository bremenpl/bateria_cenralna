#include "citemslcmenu.h"

CItemsLcMenu::CItemsLcMenu(QWidget *parent) : CItemsMenu(parent)
{
    // set the menu name
    m_menuName = "Line Controllers Menu";

    // set device type
    m_deviceType = EDeviceTypes::LineCtrler;
}

void CItemsLcMenu::slavesChangedDevSpecific(const QVector<CBcSlaveDevice*>& slaves)
{
    (void)slaves;
}




