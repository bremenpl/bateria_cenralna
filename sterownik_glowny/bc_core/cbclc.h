#ifndef CBCLC_H
#define CBCLC_H

#include <QObject>
#include "cbcslavedevice.h"
#include "cbcrc.h"

class CBcLc : public CBcSlaveDevice
{
    Q_OBJECT
public:
    explicit CBcLc(const quint16 slaveAddr,
                   const quint32 pingsMax,
                   const QVector<slaveId*>* pv = 0,
                   QObject *parent = 0);

signals:

public slots:


};

#endif // CBCLC_H



