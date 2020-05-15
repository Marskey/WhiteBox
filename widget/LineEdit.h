#pragma once
#include <QLineEdit>

class CLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    CLineEdit(QWidget* parent);
    virtual ~CLineEdit() {}

protected:
    void focusInEvent(QFocusEvent* e);
    void mousePressEvent(QMouseEvent* me);

    bool m_selectOnMousePress;
};

