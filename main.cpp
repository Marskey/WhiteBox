#include "EmulatorClient.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CEmulatorClient w;
    w.show();
    return a.exec();
}
