#include "cabstractmenu.h"
#include "ui_cabstractmenu.h"
#include "mainwindow.h"

CAbstractMenu::CAbstractMenu(QWidget *parent) : QDialog(parent), ui(new Ui::CAbstractMenu)
{
    ui->setupUi(this);

    // connect with main window
    auto mainObj = dynamic_cast<MainWindow*>(parent);
    connect(this, SIGNAL(signal_BtnClicked(const EBtnTypes)),
            mainObj, SLOT(on_MenuBtnClicked(const EBtnTypes)));
}

CAbstractMenu::~CAbstractMenu()
{
    delete ui;

    // inform about object destruction
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo) << m_menuName << " destroyed";
}
