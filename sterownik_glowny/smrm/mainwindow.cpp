#include <QIntValidator>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "cbclogger.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // setup logger
    CBcLogger::instance()->startLogger("/tmp", true, MLL::ELogLevel::LDebug);

    ui->setupUi(this);

    // create modbus master
    mp_modbusMaster =  new Csmrm(this);

    // set serial parameters
    mp_modbusMaster->setBaudRate(QSerialPort::Baud9600);
    mp_modbusMaster->setDataBits(QSerialPort::Data8);
    mp_modbusMaster->setParity(QSerialPort::EvenParity);
    mp_modbusMaster->setStopBits(QSerialPort::OneStop);
    mp_modbusMaster->setFlowControl(QSerialPort::NoFlowControl);

    // fill the combobox with available serial ports
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        ui->cbSerialPort->addItem(info.portName());

    // create validation rule for the slave device line edit
    QIntValidator* slaveValidator = new QIntValidator(1, 247, this);
    ui->leSlaveAddr->setValidator(slaveValidator);
    ui->leSlaveAddr->setText("5");

    // create validation rule for starting address
    QIntValidator* startAddrValidator = new QIntValidator(0, 8, this);
    ui->leStartAddr->setValidator(startAddrValidator);
    ui->leStartAddr->setText("0");

    // set value line edit to read only
    ui->leValue->setReadOnly(true);

    // disable modbus groupbox
    ui->gbModBusActions->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;

    if (mp_modbusMaster)
    {
        mp_modbusMaster->close();
        mp_modbusMaster->deleteLater();
    }
}

void MainWindow::on_pbConnect_clicked()
{
   if ("connect" == ui->pbConnect->text())
   {
       //assign serial port
       mp_modbusMaster->setPortName(ui->cbSerialPort->currentText());

       // try to open the port
       if (!mp_modbusMaster->open(QIODevice::ReadWrite))
       {
            CBcLogger::instance()->print(MLL::ELogLevel::LCritical)
                    << "Cannot open serial port " << mp_modbusMaster->portName();
       }
       else
       {
           ui->pbConnect->setText("disconnect");
           ui->gbModBusActions->setEnabled(true);

           // connect response slots
           connect(mp_modbusMaster, SIGNAL(responseReady_ReadHoldingRegisters(const quint8, const QVector<quint16>&)),
                   this, SLOT(on_responseReady_ReadHoldingRegisters(const quint8, const QVector<quint16>&)),
                   Qt::UniqueConnection);

           CBcLogger::instance()->print(MLL::ELogLevel::LInfo)
                   << "Serial port " << mp_modbusMaster->portName() << " opened.";
       }
   }
   else
   {
       mp_modbusMaster->close();
       ui->gbModBusActions->setEnabled(false);
       ui->pbConnect->setText("connect");

       // disconnect response slots
       disconnect(mp_modbusMaster, SIGNAL(responseReady_ReadHoldingRegisters(const quint8, const QVector<quint16>&)),
               this, SLOT(on_responseReady_ReadHoldingRegisters(const quint8, const QVector<quint16>&)));

       CBcLogger::instance()->print(MLL::ELogLevel::LInfo)
               << "Serial port " << mp_modbusMaster->portName() << " closed.";
   }
}

void MainWindow::on_pbReadHoldingRegs_clicked()
{
    // test
    mp_modbusMaster->sendRequest_ReadHoldingRegisters(ui->leSlaveAddr->text().toInt(), ui->leStartAddr->text().toInt(), 1);
}

void MainWindow::on_responseReady_ReadHoldingRegisters(const quint8 slaveId, const QVector<quint16>& registers)
{
    ui->leValue->setText(QString::number(registers.last()));
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo, "Read holding registers response arrived from slave %u", slaveId);
}


















