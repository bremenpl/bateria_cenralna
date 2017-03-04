#include "citemsbatmenu.h"

CItemsBatMenu::CItemsBatMenu(const quint8 parentAddr, QWidget *parent) :
    CItemsMenu(parentAddr, parent)
{
    // set the menu name
    m_menuName = "Batteries Menu";

    // set device type
    m_deviceType = EDeviceTypes::Battery;
}
