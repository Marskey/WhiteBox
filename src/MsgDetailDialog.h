#pragma once

#include <QDialog>
#include <ui_MessageDetail.h>
#include <QKeyEvent>

class CloseDialogFilter : public QObject
{
    Q_OBJECT
protected:
    bool eventFilter(QObject* obj, QEvent* event) {
        auto keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key::Key_Space) {
            if (keyEvent->isAutoRepeat()) {
                return true;
            }

            // 取消原先的按空格滚屏的操作
            if (event->type() == QEvent::KeyPress) {
                return true;
            }

            if (event->type() == QEvent::KeyRelease) {
                ((QDialog*)obj->parent())->close();
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

class CMsgDetailDialog : public QDialog, public Ui_MsgDetail
{
    Q_OBJECT
public:
    CMsgDetailDialog(QWidget *parent, const QString& title, const QString& detail);
    ~CMsgDetailDialog();
private:
    CloseDialogFilter* m_filter = new CloseDialogFilter;
};

