#ifndef CITEMSMENU_H
#define CITEMSMENU_H

#include <QDialog>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

#include "cabstractmenu.h"

namespace Ui {
class CItemsMenu;
}

class CItemsMenu : public CAbstractMenu
{
    Q_OBJECT

public:
    explicit CItemsMenu(QWidget *parent = 0);
    ~CItemsMenu();

    void selectNextItem();

private:
    Ui::CItemsMenu *ui;

    // members
    int             m_currentItem;

protected:
    QTreeWidget*    mp_tw;
    int             m_noOfItems;
    EDeviceTypes    m_deviceType = EDeviceTypes::Dummy;

signals:
    void signal_deviceSelected(const EDeviceTypes deviceType, const int slaveAddr);

private slots:
    void on_pbItemsUp_clicked();
    void on_pbItemsDown_clicked();
    void on_twItems_itemClicked(QTreeWidgetItem *item, int column);
    void on_pbItemsEnter_clicked();
};

#endif // CITEMSMENU_H
