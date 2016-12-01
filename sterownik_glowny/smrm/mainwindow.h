#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "csmrm.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pbConnect_clicked();
    void on_responseReady_ReadHoldingRegisters(const quint8 slaveId, const QVector<quint16>& registers);

    void on_pbReadHoldingRegs_clicked();

private:
    Ui::MainWindow *ui;

    Csmrm* mp_modbusMaster;
};

#endif // MAINWINDOW_H
