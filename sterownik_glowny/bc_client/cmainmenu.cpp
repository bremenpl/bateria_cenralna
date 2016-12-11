#include "cmainmenu.h"
#include "ui_cmainmenu.h"

CMainMenu::CMainMenu(QWidget *parent) : CAbstractMenu(parent), ui(new Ui::CMainMenu)
{
    ui->setupUi(this);

    // set the menu name
    m_menuName = "Main Menu";

    // add labels to the table view
    QStringList list;
    list << QObject::tr("Parameter")
         << QObject::tr("Value");

    ui->twStatus->setColumnCount(list.size());
    ui->twStatus->setHeaderLabels(list);

    // resize all
    ui->twStatus->header()->resizeSection(0, 300);
    ui->twStatus->header()->resizeSection(1, 200);

    // total slaves
    QTreeWidgetItem* item = new QTreeWidgetItem(ui->twStatus);
    item->setText(0, "Total devices");
    item->setText(1, "90");

    // total chargers
    item = new QTreeWidgetItem(ui->twStatus);
    item->setText(0, "Total chargers");
    item->setText(1, "6");

    // Total line controllers
    item = new QTreeWidgetItem(ui->twStatus);
    item->setText(0, "Total line controllers");
    item->setText(1, "64");

    // Total relay controllers
    item = new QTreeWidgetItem(ui->twStatus);
    item->setText(0, "Total relay controllers");
    item->setText(1, "1280");
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
