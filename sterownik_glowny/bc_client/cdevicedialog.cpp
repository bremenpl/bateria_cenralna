#include "cdevicedialog.h"
#include "ui_cdevicedialog.h"

CDeviceDialog::CDeviceDialog(QWidget *parent) : CAbstractMenu(parent), ui(new Ui::CDeviceDialog)
{
    ui->setupUi(this);

    // set the menu name
    m_menuName = "Devices Menu";
}

CDeviceDialog::~CDeviceDialog()
{
    delete ui;
}

void CDeviceDialog::on_pbDevicesBatteries_clicked()
{
    emit signal_BtnClicked(EBtnTypes::Batteries);
}

void CDeviceDialog::on_pbDevicesLineControllers_clicked()
{
    emit signal_BtnClicked(EBtnTypes::LineControllers);
}

void CDeviceDialog::on_pbDevicesPowersupply_clicked()
{
    emit signal_BtnClicked(EBtnTypes::PowerAdapter);
}

void CDeviceDialog::on_pbDevicesChargers_clicked()
{
    emit signal_BtnClicked(EBtnTypes::Chargers);
}
