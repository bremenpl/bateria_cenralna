#ifndef CBCLC_H
#define CBCLC_H

#include <QObject>

#include "cbcslavedevice.h"

class CBcLc : public CBcSlaveDevice
{
    Q_OBJECT
public:
    explicit CBcLc(QObject *parent = 0);

signals:

public slots:


};

#endif // CBCLC_H
