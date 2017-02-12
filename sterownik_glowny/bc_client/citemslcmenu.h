#ifndef CITEMSLCMENU_H
#define CITEMSLCMENU_H

#include <QWidget>
#include <QString>
#include <QStringList>

#include "citemsmenu.h"

class CItemsLcMenu : public CItemsMenu
{
    Q_OBJECT
public:
    explicit CItemsLcMenu(QWidget *parent = 0);

signals:

public slots:

private:
    void slavesChangedDevSpecific(const QVector<CBcSlaveDevice*>& slaves);
    QVector<slaveId*> getParentVector(const int row);

};

#endif // CITEMSLCMENU_H
