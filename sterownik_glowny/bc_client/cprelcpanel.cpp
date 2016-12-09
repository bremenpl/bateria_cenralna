#include "cprelcpanel.h"
#include "ui_cprelcpanel.h"

CPreLcPanel::CPreLcPanel(QWidget *parent) : CAbstractMenu(parent), ui(new Ui::CPreLcPanel)
{
    ui->setupUi(this);

    // set the menu name
    m_menuName = "Line Controller prePanel";
}

CPreLcPanel::~CPreLcPanel()
{
    delete ui;
}

void CPreLcPanel::on_pbPreLcRelayCtrlers_clicked()
{
    emit signal_BtnClicked(EBtnTypes::RelayControllers);
}
