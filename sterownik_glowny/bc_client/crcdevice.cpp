#include "crcdevice.h"


CRcDevice::CRcDevice(QWidget *parent) : CAbstractDevice(parent)
{
    // set the menu name
    m_menuName = "Relay controller device";

    // place line controllers tree view specific modifications here if needed

    // resize acces and value column
    mp_tw->header()->resizeSection(1, 80);
    mp_tw->header()->resizeSection(2, 150);

    // 1st parameter: Relay status
    QTreeWidgetItem* item = new QTreeWidgetItem(mp_tw);
    item->setText(0, "Relay status");
    item->setText(1, "RW");
    item->setText(2, "OFF");
    item->setText(3, "Indicates either the light is ON or OFF");

    // 2nd parameter: Power source (power grid or battery)
    item = new QTreeWidgetItem(mp_tw);
    item->setText(0, "Power source");
    item->setText(1, "R");
    item->setText(2, "Power grid");
    item->setText(3, "Can be powered from the grid or the battery");

    // 3rd parameter: blackout behaviour
    item = new QTreeWidgetItem(mp_tw);
    item->setText(0, "Blackout action");
    item->setText(1, "RW");
    item->setText(2, "Switch to bat");
    item->setText(3, "Can use battery power in case of emergency");

}
