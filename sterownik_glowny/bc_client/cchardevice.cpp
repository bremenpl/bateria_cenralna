#include "cchardevice.h"


CCharDevice::CCharDevice(QWidget *parent) : CAbstractDevice(parent)
{
    // set the menu name
    m_menuName = "charger device";

    // place charger tree view specific modifications here if needed

	// resize name and access column
    mp_tw->header()->resizeSection(0, 250);
    mp_tw->header()->resizeSection(2, 150);

    // 1st parameter: voltage
    QTreeWidgetItem* item = new QTreeWidgetItem(mp_tw);
    item->setText(0, "Charging voltage [V]");
    item->setText(1, "R");
    item->setText(2, "24");

    // 2nd parameter: current
    item = new QTreeWidgetItem(mp_tw);
    item->setText(0, "Charging current [A]");
    item->setText(1, "R");
    item->setText(2, "0.5");
	
	// 3rd parameter: power
    item = new QTreeWidgetItem(mp_tw);
    item->setText(0, "Charging limit [s]");
    item->setText(1, "R");
    item->setText(2, "7200");
}
