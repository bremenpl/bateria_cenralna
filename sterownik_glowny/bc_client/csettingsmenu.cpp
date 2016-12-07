#include "csettingsmenu.h"
#include "ui_csettingsmenu.h"

CSettingsMenu::CSettingsMenu(QWidget *parent) : CAbstractMenu(parent), ui(new Ui::CSettingsMenu)
{
    ui->setupUi(this);

    // set the menu name
    m_menuName = "Settings Menu";
}

CSettingsMenu::~CSettingsMenu()
{
    delete ui;
}
