#ifndef CMAINMENU_H
#define CMAINMENU_H

#include <QDialog>
#include <QMainWindow>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>

#include "cabstractmenu.h"
#include "cbcslavedevice.h"

enum class slaveRows
{
    total       = 0,
    lcs         = 1,
    rcs         = 2,
};

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
    void on_slavesChanged(const QVector<CBcSlaveDevice*>& slaves);

    void on_pbSettings_clicked();
    void on_pbDevices_clicked();

private:
    Ui::CMainMenu *ui;
    void addSlavesGroup(const QString& name, const int amount);
    void changeSlavesGroupAmount(const slaveRows slaves, const int amount);

    QStandardItemModel* mp_itemsModel;
    QStringList         m_columnLabels;
};

#endif // CMAINMENU_H
