#include "csettingsmenu.h"
#include "ui_csettingsmenu.h"

CSettingsMenu::CSettingsMenu(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CSettingsMenu)
{
    ui->setupUi(this);
}

CSettingsMenu::~CSettingsMenu()
{
    delete ui;
}
