#ifndef CITEMSMENU_H
#define CITEMSMENU_H

#include <QDialog>
#include <QStandardItemModel>
#include <QItemSelection>

#include "cabstractmenu.h"

enum class itemsTableCol
{
    slaveAddr       = 0,
    uniqId          = 1,
    userStr         = 2
};

namespace Ui {
class CItemsMenu;
}

class CItemsMenu : public CAbstractMenu
{
    Q_OBJECT

public:
    explicit CItemsMenu(QWidget *parent = 0);
    ~CItemsMenu();

private:
    Ui::CItemsMenu *ui;
    void addSlave(const quint8 addr, const QString& uniqId, const QString& userString);
    void selectNextItem(const bool nextItem);
    int selectedRow();
    void manageControls();

    // members
    QStringList     m_columnLabels;

protected:
    int             m_noOfItems;
    EDeviceTypes    m_deviceType = EDeviceTypes::Dummy;
    QStandardItemModel* mp_itemsModel;

signals:
    void signal_deviceSelected(const EDeviceTypes deviceType, const int slaveAddr);

private slots:
    void on_pbItemsUp_clicked();
    void on_pbItemsDown_clicked();
    void on_pbItemsEnter_clicked();
    void on_itemsSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

};

#endif // CITEMSMENU_H
