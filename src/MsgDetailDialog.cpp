#include "MsgDetailDialog.h"
#include "MainWindow.h"
#include "fmt/format.h"
#include "JsonHighlighter.h"

CMsgDetailDialog::CMsgDetailDialog(QWidget* parent, const QString& title, const QString& detail) : QDialog(parent) {
    setupUi(this);
    setWindowTitle(title);

    m_highlighter = new CJsonHighlighter(textEdit->document());

    // 如果为空
    textEdit->setText(detail.toStdString().c_str());
    textEdit->installEventFilter(m_filter);
}

CMsgDetailDialog::~CMsgDetailDialog() {
}
