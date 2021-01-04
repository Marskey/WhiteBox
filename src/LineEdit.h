#pragma once
#include <QLineEdit>
#include <QStyledItemDelegate>

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

class PopupItemDelegate : public QStyledItemDelegate
{
public:
  using QStyledItemDelegate::QStyledItemDelegate;
  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    s.setHeight(20);
    return s;
  }
};
