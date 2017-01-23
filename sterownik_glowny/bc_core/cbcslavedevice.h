#ifndef CBCSLAVEDEVICE_H
#define CBCSLAVEDEVICE_H

#include <QObject>

class CBcSlaveDevice : public QObject
{
    Q_OBJECT
public:
    explicit CBcSlaveDevice(QObject *parent = 0);

signals:

public slots:

private:

protected:
    // members
    quint32     pings;
    bool        presence;
};

#endif // CBCSLAVEDEVICE_H
