#ifndef CDEVICEDIALOG_H
#define CDEVICEDIALOG_H

#include <QDialog>

namespace Ui {
class CDeviceDialog;
}

class CDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CDeviceDialog(QWidget *parent = 0);
    ~CDeviceDialog();

private:
    Ui::CDeviceDialog *ui;
};

#endif // CDEVICEDIALOG_H
