#ifndef CPADEVICE_H
#define CPADEVICE_H

#include <QWidget>

#include "cabstractdevice.h"

class CPaDevice : public CAbstractDevice
{
    Q_OBJECT
public:
    explicit CPaDevice(QWidget *parent = 0);
};

#endif // CPSDEVICE_H
