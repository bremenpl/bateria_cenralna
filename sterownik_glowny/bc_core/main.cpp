#include <QCoreApplication>
#include <QCommandLineParser>
#include <QString>
#include <QDebug>
#include <QDir>

#include "cbcmain.h"

int main(int argc, char *argv[])
{
    QCoreApplication coreApp(argc, argv);
    CBcMain* mainObj = 0;

    // create the main object
    mainObj = new CBcMain(coreApp);

    delete mainObj;
    return 0;
}


