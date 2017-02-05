#include "citemsbatmenu.h"

CItemsBatMenu::CItemsBatMenu(QWidget *parent) : CItemsMenu(parent)
{
    // set the menu name
    m_menuName = "Batteries Menu";

    // set device type
    m_deviceType = EDeviceTypes::Battery;
}
