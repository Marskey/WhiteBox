#include "MainWindow.h"
#include "ConfigHelper.h"

#include <QtWidgets/QApplication>
#include <QStyleFactory>

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

#if 1
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette p = qApp->palette();
    p.setColor(QPalette::Window, QColor(53, 53, 53));
    p.setColor(QPalette::WindowText, Qt::white);
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
    p.setColor(QPalette::Base, QColor(42, 42, 42));
    p.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
    p.setColor(QPalette::ToolTipBase, Qt::white);
    p.setColor(QPalette::ToolTipText, Qt::white);
    p.setColor(QPalette::Text, Qt::white);
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    p.setColor(QPalette::Dark, QColor(35, 35, 35));
    p.setColor(QPalette::Shadow, QColor(20, 20, 20));
    p.setColor(QPalette::Button, QColor(53, 53, 53));
    p.setColor(QPalette::ButtonText, Qt::white);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, QColor(42, 130, 218));
    p.setColor(QPalette::Highlight, QColor(42, 130, 218));
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));
    qApp->setPalette(p);
#endif

    CMainWindow w;
    w.init();
    w.show();
    return a.exec();
}
