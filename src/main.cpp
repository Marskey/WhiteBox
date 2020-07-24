#include "MainWindow.h"
#include "ConfigHelper.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    ConfigHelper::instance().init("config.ini");

    QApplication a(argc, argv);
    QString f = ConfigHelper::instance().getFont();
    QFont font = a.font();
    if (!f.isEmpty()) {
        font.setFamily(f);
    } else {
        ConfigHelper::instance().saveFont(font.family());
    }
    a.setFont(font);
    CMainWindow w;
    w.init();
    w.show();
    return a.exec();
}
