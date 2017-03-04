#ifndef CITEMSRCMENU_H
#define CITEMSRCMENU_H

#include <QWidget>

#include "citemsmenu.h"

class CItemsRcMenu : public CItemsMenu
{
    Q_OBJECT
public:
    explicit CItemsRcMenu(const quint8 parentAddr, QWidget *parent = 0);

signals:

public slots:

private:
    void slavesChangedDevSpecific(const QVector<CBcSlaveDevice*>& slaves);
    QVector<slaveId*> getParentVector(const int row);
};

#endif // CITEMSRCMENU_H
