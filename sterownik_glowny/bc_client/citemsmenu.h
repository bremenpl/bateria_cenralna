#ifndef CITEMSMENU_H
#define CITEMSMENU_H

#include <QDialog>
#include <QStandardItemModel>
#include <QItemSelection>
#include <QVector>

#include "cabstractmenu.h"
#include "cbcslavedevice.h"

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

    QString getUniqIdString(const quint16* const uniqId);
    void addSlave(const quint8 addr, const QString& uniqId, const QString& userString);

private:
    Ui::CItemsMenu *ui;
    void selectNextItem(const bool nextItem);
    int selectedRow();
    void manageControls();
    virtual void slavesChangedDevSpecific(const QVector<CBcSlaveDevice*>& slaves);
    virtual QVector<slaveId*> getParentVector(const int row);

    // members
    QStringList     m_columnLabels;
    QVector<CBcSlaveDevice*>*   mp_slaves = 0;

protected:
    int             m_noOfItems;
    EDeviceTypes    m_deviceType = EDeviceTypes::Dummy;
    QStandardItemModel* mp_itemsModel;

signals:
    void deviceSelected(const EDeviceTypes deviceType, const int slaveAddr);
    void getSlaveUniqId(const QVector<slaveId*>& pv);

private slots:
    void on_slavesChanged(const QVector<CBcSlaveDevice*>& slaves);

    void on_pbItemsUp_clicked();
    void on_pbItemsDown_clicked();
    void on_pbItemsEnter_clicked();
    void on_itemsSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

};

#endif // CITEMSMENU_H
