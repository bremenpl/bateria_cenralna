#include "cbatteriesmenu.h"
#include "ui_cbatteriesmenu.h"

CBatteriesMenu::CBatteriesMenu(QWidget *parent) : CAbstractMenu(parent), ui(new Ui::CBatteriesMenu)
{
    ui->setupUi(this);

    // set the menu name
    m_menuName = "Batteries Menu";
}

CBatteriesMenu::~CBatteriesMenu()
{
    delete ui;
}
