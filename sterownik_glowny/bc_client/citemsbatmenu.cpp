#include "citemsbatmenu.h"

CItemsBatMenu::CItemsBatMenu(QWidget *parent) : CItemsMenu(parent)
{
    // set the menu name
    m_menuName = "Batteries Menu";

    // set device type
    m_deviceType = EDeviceTypes::Battery;

    // place line controllers tree view specific modifications here if needed

    // add some dummy items TEMP
    for (int i = 1; i <= 19; i++)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(mp_tw);
        item->setText(0, QString::number(i));
        QString sn = "0x" + QString::number(qrand(), 16);
        item->setText(1, sn);
        m_noOfItems++;
    }

    selectNextItem();
}
