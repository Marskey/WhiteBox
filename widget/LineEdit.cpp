#include "LineEdit.h"

CLineEdit::CLineEdit(QWidget* parent)
    :m_selectOnMousePress(false) {
}

void CLineEdit::focusInEvent(QFocusEvent* e) {
    QLineEdit::focusInEvent(e);
    selectAll();
    m_selectOnMousePress = true;
}

void CLineEdit::mousePressEvent(QMouseEvent* me) {
    QLineEdit::mousePressEvent(me);
    if (m_selectOnMousePress) {
        selectAll();
        m_selectOnMousePress = false;
    }
}
