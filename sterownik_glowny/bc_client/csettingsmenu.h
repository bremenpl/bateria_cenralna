#ifndef CSETTINGSMENU_H
#define CSETTINGSMENU_H

#include <QDialog>

#include "cabstractmenu.h"

namespace Ui {
class CSettingsMenu;
}

class CSettingsMenu : public CAbstractMenu
{
    Q_OBJECT

public:
    explicit CSettingsMenu(QWidget *parent = 0);
    ~CSettingsMenu();

private:
    Ui::CSettingsMenu *ui;


};

#endif // CSETTINGSMENU_H
