#ifndef CBATTERIESMENU_H
#define CBATTERIESMENU_H

#include <QDialog>

#include "cabstractmenu.h"

namespace Ui {
class CBatteriesMenu;
}

class CBatteriesMenu : public CAbstractMenu
{
    Q_OBJECT

public:
    explicit CBatteriesMenu(QWidget *parent = 0);
    ~CBatteriesMenu();

private:
    Ui::CBatteriesMenu *ui;
};

#endif // CBATTERIESMENU_H
