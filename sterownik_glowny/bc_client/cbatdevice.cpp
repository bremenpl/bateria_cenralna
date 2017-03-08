#include "cbatdevice.h"


CBatDevice::CBatDevice(QWidget *parent) : CAbstractDevice(parent)
{
    // set the menu name
    m_menuName = "Battery device";

    // place line controllers tree view specific modifications here if needed

    // resize parameter, acces and value column
    /*mp_tw->header()->resizeSection(0, 250);
    mp_tw->header()->resizeSection(1, 80);
    mp_tw->header()->resizeSection(2, 150);

    // 1st parameter: Voltage
    QTreeWidgetItem* item = new QTreeWidgetItem(mp_tw);
    item->setText(0, "Voltage [V]");
    item->setText(1, "R");
    item->setText(2, "12");

    // 2nd parameter: Temperature
    item = new QTreeWidgetItem(mp_tw);
    item->setText(0, "Temperature [°C]");
    item->setText(1, "R");
    item->setText(2, "40");

    // 3rd paameter: capacity
    item = new QTreeWidgetItem(mp_tw);
    item->setText(0, "Capacity [Ah]");
    item->setText(1, "R");
    item->setText(2, "60");*/
}
