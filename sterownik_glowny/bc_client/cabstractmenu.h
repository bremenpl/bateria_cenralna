#ifndef CABSTRACTMENU_H
#define CABSTRACTMENU_H

#include <QDialog>

#include "cbclogger.h"
#include "types.h"

namespace Ui {
class CAbstractMenu;
}

class CAbstractMenu : public QDialog
{
    Q_OBJECT

public:
    explicit CAbstractMenu(QWidget *parent = 0);
    ~CAbstractMenu();

    const QString& menuName(){ return m_menuName; }

signals:
    void signal_BtnClicked(const EBtnTypes btn);

private:
    Ui::CAbstractMenu *ui;

protected:
    QString     m_menuName = "Abstract Menu";
};

#endif // CABSTRACTMENU_H
