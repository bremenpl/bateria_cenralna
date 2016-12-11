#ifndef CBATDEVICE_H
#define CBATDEVICE_H

#include <QWidget>

#include "cabstractdevice.h"

class CBatDevice : public CAbstractDevice
{
    Q_OBJECT
public:
    explicit CBatDevice(QWidget *parent = 0);
};

#endif // CBATDEVICE_H
