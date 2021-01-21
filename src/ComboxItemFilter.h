#pragma once
#include <qabstractitemview.h>
#include <QKeyEvent>
#include <qobject.h>

class DeleteHighlightedItemFilter : public QObject
{
  Q_OBJECT
public:
  DeleteHighlightedItemFilter(QObject* parent = nullptr) :QObject(parent) {}
protected:
  bool eventFilter(QObject* obj, QEvent* event) override {
    if (event->type() == QEvent::KeyPress) {
      auto keyEvent = static_cast<QKeyEvent*>(event);
      if (keyEvent->modifiers() == Qt::ShiftModifier && keyEvent->key() == Qt::Key::Key_Delete) {
        auto itemView = qobject_cast<QAbstractItemView*>(obj);
        if (itemView->model() != nullptr) {
          // Delete the current item from the popup list
          itemView->model()->removeRow(itemView->currentIndex().row());
          return true;
        }
      }
    }
    // Perform the usual event processing
    return QObject::eventFilter(obj, event);
  }
};
