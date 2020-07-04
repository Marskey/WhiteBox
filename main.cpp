#include "MainWindow.h"
#include "ConfigHelper.h"
#include "windows.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    AllocConsole();
    freopen("CONOUT$", "w+t", stdout);

    ConfigHelper::singleton().init("config.ini");

    QApplication a(argc, argv);
    CMainWindow w;
    w.init();
    w.show();
    return a.exec();
}
