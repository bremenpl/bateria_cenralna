#ifndef CSETTINGSMENU_H
#define CSETTINGSMENU_H

#include <QDialog>

namespace Ui {
class CSettingsMenu;
}

class CSettingsMenu : public QDialog
{
    Q_OBJECT

public:
    explicit CSettingsMenu(QWidget *parent = 0);
    ~CSettingsMenu();

private:
    Ui::CSettingsMenu *ui;
};

#endif // CSETTINGSMENU_H
