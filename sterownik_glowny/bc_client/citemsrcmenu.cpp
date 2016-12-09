#include "citemsrcmenu.h"

CItemsRcMenu::CItemsRcMenu(QWidget *parent) : CItemsMenu(parent)
{
    // set the menu name, it has to be replaced anyways
    m_menuName = "Relay Controllers Menu";

    // set device type
    m_deviceType = EDeviceTypes::RelayCtrler;

    // place line controllers tree wiev specific modifications here if needed

    // add some dummy items TEMP
    for (int i = 1; i <= 20; i++)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(mp_tw);
        item->setText(0, QString::number(i));
        QString sn = "0x" + QString::number(qrand(), 16);
        item->setText(1, sn);
        m_noOfItems++;
    }

    selectNextItem();
}
