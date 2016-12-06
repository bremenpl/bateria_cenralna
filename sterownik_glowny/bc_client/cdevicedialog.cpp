#include "cdevicedialog.h"
#include "ui_cdevicedialog.h"

CDeviceDialog::CDeviceDialog(QWidget *parent) : QDialog(parent), ui(new Ui::CDeviceDialog)
{
    ui->setupUi(this);


}

CDeviceDialog::~CDeviceDialog()
{
    delete ui;
}
