#ifndef CDEVICEDIALOG_H
#define CDEVICEDIALOG_H

#include <QDialog>

#include "cabstractmenu.h"

namespace Ui {
class CDeviceDialog;
}

class CDeviceDialog : public CAbstractMenu
{
    Q_OBJECT

public:
    explicit CDeviceDialog(QWidget *parent = 0);
    ~CDeviceDialog();

private slots:
    void on_pbDevicesBatteries_clicked();

    void on_pbDevicesLineControllers_clicked();

    void on_pbDevicesPowersupply_clicked();

    void on_pbDevicesChargers_clicked();

private:
    Ui::CDeviceDialog *ui;

};

#endif // CDEVICEDIALOG_H
