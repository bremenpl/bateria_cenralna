#include "cabstractdevice.h"
#include "ui_cabstractdevice.h"

CAbstractDevice::CAbstractDevice(QWidget *parent) : CAbstractMenu(parent), ui(new Ui::CAbstractDevice)
{
    ui->setupUi(this);

    // set the menu name, it has to be replaced anyways
    m_menuName = "Astract device menu";

    // add generic labels to the table view
    QStringList list;
    list << QObject::tr("Parameter")
         << QObject::tr("Access")
         << QObject::tr("Value")
         << QObject::tr("Description");

    ui->twParams->setColumnCount(list.size());
    ui->twParams
            ->setHeaderLabels(list);

    // resize all
    for (int i = 0; i < ui->twParams->columnCount(); i++)
        ui->twParams->header()->resizeSection(i, 200);

    // share tree widget
    mp_tw = ui->twParams;
}

CAbstractDevice::~CAbstractDevice()
{
    delete ui;
}
