#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // setup logger
    CBcLogger::instance()->startLogger("/tmp", true, MLL::ELogLevel::LDebug);

    // create socket object
    mp_tcpSocket = new QTcpSocket(this);

    // connect slots
    connect(mp_tcpSocket, SIGNAL(connected()),
            this, SLOT(on_tcpSocketConnected()));
    connect(mp_tcpSocket, SIGNAL(disconnected()),
            this, SLOT(on_tcpSocketDisconnected()));
    connect(mp_tcpSocket, SIGNAL(readyRead()),
            this, SLOT(on_tcpSocketReadyRead()));

    CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                 "Trying to connect to host %s:%u...", HOST_IP, HOST_PORT);

    mp_tcpSocket->connectToHost(HOST_IP, HOST_PORT);

    // check connection
    if (!mp_tcpSocket->waitForConnected(1000))
    {
        CBcLogger::instance()->print(MLL::ELogLevel::LCritical,
                                     "Unable to connect to host %s:%u, quiting", HOST_IP, HOST_PORT);
        QThread::usleep(500);
        exit(1);
    }

}

MainWindow::~MainWindow()
{
    delete ui;

    if (mp_tcpSocket)
        mp_tcpSocket->disconnect();
}

void MainWindow::on_tcpSocketConnected()
{
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                 "Connected to host %s:%u...", HOST_IP, HOST_PORT);
}


void MainWindow::on_tcpSocketDisconnected()
{
    CBcLogger::instance()->print(MLL::ELogLevel::LInfo,
                                 "Disconnected from host host %s:%u...", HOST_IP, HOST_PORT);
    QThread::usleep(500);
    exit(1);
}

void MainWindow::on_tcpSocketReadyRead()
{
    QByteArray rb = mp_tcpSocket->readAll();
    quint16 state = rb[0] | ((quint16)rb[1] << 8);

    ui->lLedState->setText(QString::number(state));
}













