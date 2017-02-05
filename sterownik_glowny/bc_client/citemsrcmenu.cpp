#include "citemsrcmenu.h"

CItemsRcMenu::CItemsRcMenu(QWidget *parent) : CItemsMenu(parent)
{
    // set the menu name, it has to be replaced anyways
    m_menuName = "Relay Controllers Menu";

    // set device type
    m_deviceType = EDeviceTypes::RelayCtrler;
}
