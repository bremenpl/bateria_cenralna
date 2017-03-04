#include "citemscharmenu.h"

CItemsCharMenu::CItemsCharMenu(const quint8 parentAddr, QWidget *parent) :
    CItemsMenu(parentAddr, parent)
{
    // set the menu name
    m_menuName = "Chargers Menu";

    // set device type
    m_deviceType = EDeviceTypes::Charger;
}
