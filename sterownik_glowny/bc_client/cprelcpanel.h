#ifndef CPRELCPANEL_H
#define CPRELCPANEL_H

#include <QDialog>

#include "cabstractmenu.h"

namespace Ui {
class CPreLcPanel;
}

class CPreLcPanel : public CAbstractMenu
{
    Q_OBJECT

public:
    explicit CPreLcPanel(QWidget *parent = 0);
    ~CPreLcPanel();

private slots:
    void on_pbPreLcRelayCtrlers_clicked();

private:
    Ui::CPreLcPanel *ui;
};

#endif // CPRELCPANEL_H
