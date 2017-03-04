#ifndef CITEMSCHARMENU_H
#define CITEMSCHARMENU_H

#include <QWidget>

#include "citemsmenu.h"

class CItemsCharMenu : public CItemsMenu
{
    Q_OBJECT
public:
    explicit CItemsCharMenu(const quint8 parentAddr, QWidget *parent = 0);

signals:

public slots:

private:

};

#endif // CITEMSBATMENU_H
