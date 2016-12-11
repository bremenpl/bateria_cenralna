#ifndef CABSTRACTDEVICE_H
#define CABSTRACTDEVICE_H

#include <QDialog>
#include <QTreeWidget>
#include <QHeaderView>

#include "cabstractmenu.h"

namespace Ui {
class CAbstractDevice;
}

class CAbstractDevice : public CAbstractMenu
{
    Q_OBJECT

public:
    explicit CAbstractDevice(QWidget *parent = 0);
    ~CAbstractDevice();

private:
    Ui::CAbstractDevice *ui;

protected:
    QTreeWidget*    mp_tw;
};

#endif // CABSTRACTDEVICE_H
