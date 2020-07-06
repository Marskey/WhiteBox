#include "MsgDetailDialog.h"
#include "MainWindow.h"
#include "fmt/format.h"

CMsgDetailDialog::CMsgDetailDialog(QWidget* parent, const QString& title, const QString& detail) : QDialog(parent) {
    setupUi(this);
    setWindowTitle(title);

    std::string content;
    // 如果为空
    if (detail == "{}\n") {
        content = fmt::format("<!DOCTYPE html><html><body><div><h2 style=\"text-align: center; color: #282c34\">No content</h2></div></body></html>");
    } else {
        content = fmt::format("<!DOCTYPE html><html><body><pre>{}</pre></body></html>"
                              , CMainWindow::highlightJsonData(detail)).c_str();
    }

    textEdit->setHtml(fmt::format("<!DOCTYPE html><html><body><pre>{}</pre></body></html>", content).c_str());
    textEdit->installEventFilter(m_filter);
}

CMsgDetailDialog::~CMsgDetailDialog() {
}
