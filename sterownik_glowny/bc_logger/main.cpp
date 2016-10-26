#include <QCoreApplication>

#include "cbclogger.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CBcLogger::instance()->startLogger("test", true);

    CBcLogger::instance()->print(ELogLevel::e_Debug, "abc %u 0x%X", 5, 99);

    return a.exec();
    //return 0;
}





