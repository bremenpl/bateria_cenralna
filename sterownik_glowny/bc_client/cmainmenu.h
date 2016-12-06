#ifndef CMAINMENU_H
#define CMAINMENU_H

#include <QDialog>
#include <QMainWindow>
#include "mainwindow.h"

namespace Ui {
class CMainMenu;
}

class CMainMenu : public QDialog
{
    Q_OBJECT

public:
    explicit CMainMenu(QWidget *parent = 0);
    ~CMainMenu();

private slots:
    void on_pbSettings_clicked();

private:
    Ui::CMainMenu *ui;

    Ui::MainWindow*    mp_mainWindow;      /*!< Main window parent */
};

#endif // CMAINMENU_H
