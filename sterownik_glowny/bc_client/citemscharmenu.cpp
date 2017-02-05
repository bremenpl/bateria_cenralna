#include "citemscharmenu.h"

CItemsCharMenu::CItemsCharMenu(QWidget *parent) : CItemsMenu(parent)
{
    // set the menu name
    m_menuName = "Chargers Menu";

    // set device type
    m_deviceType = EDeviceTypes::Charger;
}
