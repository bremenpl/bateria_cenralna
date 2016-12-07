#include "cmainmenu.h"
#include "ui_cmainmenu.h"

CMainMenu::CMainMenu(QWidget *parent) : CAbstractMenu(parent), ui(new Ui::CMainMenu)
{
    ui->setupUi(this);

    // set the menu name
    m_menuName = "Main Menu";
}

CMainMenu::~CMainMenu()
{
    delete ui;
}

void CMainMenu::on_pbSettings_clicked()
{
    emit signal_BtnClicked(EBtnTypes::Settings);
}

void CMainMenu::on_pbDevices_clicked()
{
    emit signal_BtnClicked(EBtnTypes::Devices);
}
