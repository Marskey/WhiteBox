#pragma once

#include <QStyledItemDelegate>

#include "ui_MessageEditor.h"

#include "google/protobuf/message.h"
#include <google/protobuf/util/json_util.h>


/* --------------------------------------------------------------------------------
* Property editor delegate.
* -------------------------------------------------------------------------------- */
class QtTreeViewItemDelegate : public QStyledItemDelegate
{
public:
  QtTreeViewItemDelegate(QWidget* parent = 0) : QStyledItemDelegate(parent) {}

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const Q_DECL_OVERRIDE;
  void setEditorData(QWidget* editor, const QModelIndex& index) const Q_DECL_OVERRIDE;
  void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const Q_DECL_OVERRIDE;
  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const Q_DECL_OVERRIDE;
  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
protected:
  bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) Q_DECL_OVERRIDE;
private:
  QModelIndex m_lastClickedIndex;
};

class CJsonHighlighter;
class CLineEdit;
class QFormLayout;
class CMsgEditorDialog : public QDialog, public Ui_MessageEditor
{
    Q_OBJECT

    enum class ETabIdx : int
    {
        eGUI = 0,
        eJSON = 1,
    };
public:
    CMsgEditorDialog(QWidget* parent = 0);
    ~CMsgEditorDialog();

    void initDialogByMessage(const ::google::protobuf::Message& message);
    const ::google::protobuf::Message& getMessage();


private:
    void updateGUIData();

private slots:
    void handleApplyButtonClicked();
    void treeViewDataChanged();
    void handleJsonBrowseBtnClicked();
    void handleJsonParseBtnClicked();
    void handleTabBarClicked(int index);
    void handleTextEditTextChange();
    void handleCustomContextMenuRequested(const QPoint& pos);
private:
    ::google::protobuf::Message* m_pMessage = nullptr;
    CJsonHighlighter* m_highlighter;
    bool m_isAnyType = false;
    QtTreeViewItemDelegate m_delegate;
};
