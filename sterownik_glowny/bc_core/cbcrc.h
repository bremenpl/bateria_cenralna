#ifndef CBCRC_H
#define CBCRC_H

#include <QObject>
#include "cbcslavedevice.h"

class CBcRc : public CBcSlaveDevice
{
    Q_OBJECT
public:
    explicit CBcRc(const quint16 slaveAddr, const quint32 pingsMax, QObject *parent = 0);

signals:

public slots:
};

#endif // CBCRC_H
