#include <QCoreApplication>

#include "cbclogger.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CBcLogger::instance()->startLogger("test", true);

    CBcLogger::instance()->print(CBcLogger::ELogLevel::e_Debug, "abc");

    //return a.exec();
    return 0;
}
