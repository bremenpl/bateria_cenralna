#ifndef CMAINMENU_H
#define CMAINMENU_H

#include <QDialog>
#include <QMainWindow>

#include "cabstractmenu.h"

namespace Ui {
class CMainMenu;
}

class CMainMenu : public CAbstractMenu
{
    Q_OBJECT

public:
    explicit CMainMenu(QWidget *parent = 0);
    ~CMainMenu();

private slots:
    void on_pbSettings_clicked();

    void on_pbDevices_clicked();

private:
    Ui::CMainMenu *ui;
};

#endif // CMAINMENU_H
