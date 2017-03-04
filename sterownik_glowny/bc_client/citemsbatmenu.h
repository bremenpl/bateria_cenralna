#ifndef CITEMSBATMENU_H
#define CITEMSBATMENU_H

#include <QWidget>

#include "citemsmenu.h"

class CItemsBatMenu : public CItemsMenu
{
    Q_OBJECT
public:
    explicit CItemsBatMenu(const quint8 parentAddr, QWidget *parent = 0);

signals:

public slots:

private:

};

#endif // CITEMSBATMENU_H
