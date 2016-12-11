#ifndef CRCDEVICE_H
#define CRCDEVICE_H

#include <QWidget>

#include "cabstractdevice.h"

class CRcDevice : public CAbstractDevice
{
    Q_OBJECT
public:
    explicit CRcDevice(QWidget *parent = 0);
};

#endif // CRCDEVICE_H
