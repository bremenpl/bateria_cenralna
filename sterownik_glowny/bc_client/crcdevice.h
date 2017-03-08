#ifndef CRCDEVICE_H
#define CRCDEVICE_H

#include <QWidget>
#include <QPushButton>

#include "cabstractdevice.h"

enum class statusRegBits
{
    RS          = 0,
    CD          = 3,
};

enum class paramTableRow
{
    RS          = 0,
    CD          = 1,
};

class CRcDevice : public CAbstractDevice
{
    Q_OBJECT
public:
    explicit CRcDevice(QWidget *parent = 0);

private:
    QPushButton*     mp_relayBtn;
    virtual void rowSelected(const int row);

private slots:
    void on_relayBtnPressed(bool checked = 0);
    virtual void on_slavesChanged(const QVector<CBcSlaveDevice*>& slaves);

signals:
    void getRcStatusReg(const QVector<slaveId*>* pv);
    void setRcRelayState(bool state, const QVector<slaveId*>* pv);
};

#endif // CRCDEVICE_H
