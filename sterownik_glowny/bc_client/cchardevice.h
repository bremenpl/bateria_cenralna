#ifndef CCHARDEVICE_H
#define CCHARDEVICE_H

#include <QWidget>

#include "cabstractdevice.h"

class CCharDevice : public CAbstractDevice
{
    Q_OBJECT
public:
    explicit CCharDevice(QWidget *parent = 0);
};

#endif // CCHARDEVICE_H
