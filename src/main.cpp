#include "MainWindow.h"
#include "ConfigHelper.h"
#include "windows.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
#ifdef _DEBUG
    //AllocConsole();
    //freopen("CONOUT$", "w+t", stdout);
#endif

    ConfigHelper::instance().init("config.ini");

    QApplication a(argc, argv);
    CMainWindow w;
    w.init();
    w.show();
    return a.exec();
}
