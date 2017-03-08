#ifndef CABSTRACTDEVICE_H
#define CABSTRACTDEVICE_H

#include <QDialog>
#include <QStandardItemModel>
#include <QTableView>
#include <QStringList>
#include <QVector>

#include "cabstractmenu.h"
#include "cbcslavedevice.h"

enum class paramsTableCol
{
    name        = 0,
    access      = 1,
    value       = 2
};

namespace Ui {
class CAbstractDevice;
}

class CAbstractDevice : public CAbstractMenu
{
    Q_OBJECT

public:
    explicit CAbstractDevice(QWidget *parent = 0);
    ~CAbstractDevice();

private:
    Ui::CAbstractDevice *ui;

    virtual void rowSelected(const int row);

private slots:
    void on_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    virtual void on_slavesChanged(const QVector<CBcSlaveDevice*>& slaves);

protected:
    QStandardItemModel* mp_itemsModel;
    QTableView*         mp_tv;
    QStringList m_columnLabels;
    QVector<slaveId*>*  mp_pv = 0;
};

#endif // CABSTRACTDEVICE_H
