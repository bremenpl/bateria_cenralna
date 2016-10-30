#include <QCoreApplication>
#include <QTimer>

#include "cbclogger.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CBcLogger::instance()->startLogger("test", true, MLL::ELogLevel::LDebug);

    CBcLogger::instance()->print(MLL::ELogLevel::LDebug, "abc %u 0x%X", 5, 99);

    QString test = "mama tata i ja";
    QTimer::singleShot(300, [&]{ CBcLogger::instance()->print(MLL::ELogLevel::LDebug, "1 %u 0x%X %s", 5, 99,
                                                              test.toStdString().c_str()); });
    QTimer::singleShot(250, [&]{ CBcLogger::instance()->print(MLL::ELogLevel::LDebug, "2 %u 0x%X", 5, 99); });
    QTimer::singleShot(200, [&]{ CBcLogger::instance()->print(MLL::ELogLevel::LDebug, "3 %u 0x%X", 5, 99); });

    return a.exec();
    //return 0;
}





