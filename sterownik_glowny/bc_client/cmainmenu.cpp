#include "cmainmenu.h"
#include "ui_cmainmenu.h"
#include "csettingsmenu.h"

CMainMenu::CMainMenu(QWidget *parent) : QDialog(parent), ui(new Ui::CMainMenu)
{
    ui->setupUi(this);

    // assign parent
}

CMainMenu::~CMainMenu()
{
    delete ui;
}

void CMainMenu::on_pbSettings_clicked()
{

}
