#include "MsgEditorDialog.h"
#include "LineEdit.h"
#include "JsonHighlighter.h"

#include <QListWidget>
#include <QComboBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <qpainter.h>
#include <QTextEdit>

#include "ConfigHelper.h"
#include "ProtoManager.h"
#include "ProtoTreeModel.h"

class ProtoTreeModel;

CMsgEditorDialog::CMsgEditorDialog(QWidget* parent)
  : QDialog(parent, Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint) {
  setupUi(this);

  connect(btnBrowse, SIGNAL(clicked()), this, SLOT(handleJsonBrowseBtnClicked()));
  connect(btnParse, SIGNAL(clicked()), this, SLOT(handleJsonParseBtnClicked()));
  connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(handleTabBarChanged(int)));
  connect(textEdit, SIGNAL(textChanged()), this, SLOT(handleTextEditTextChange()));
  connect(okButton, SIGNAL(clicked()), this, SLOT(handleApplyButtonClicked()));
  connect(treeView, SIGNAL(expanded(const QModelIndex&)), this, SLOT(handleTreeViewExpanded(const QModelIndex&)));

  m_highlighter = new CJsonHighlighter(textEdit->document());
  treeView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(treeView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(handleCustomContextMenuRequested(const QPoint&)));
}

CMsgEditorDialog::~CMsgEditorDialog() {
  delete m_pMessage;
  m_pMessage = nullptr;
}

void CMsgEditorDialog::initDialogByMessage(const ::google::protobuf::Message& message) {
  m_pMessage = message.New();
  m_pMessage->CopyFrom(message);

  m_isAnyType = "google.protobuf.Any" == m_pMessage->GetTypeName();

  // 设置标题
  labelTitle->setText(m_pMessage->GetTypeName().c_str());

  if (!m_isAnyType) {
    QAbstractItemModel* oldModel = treeView->model();
    if (oldModel != nullptr) {
      oldModel->disconnect(SIGNAL(dataChanged(QModelIndex, QModelIndex)));
    }

    auto* model = new ProtoTreeModel(message);
    QItemSelectionModel* selectionModel = treeView->selectionModel();
    connect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(treeViewDataChanged()));
    treeView->setItemDelegate(&m_delegate);
    treeView->setModel(model);
    treeView->reset();
    delete selectionModel;
    delete oldModel;

    treeView->expandAll();
    for (int i = 1; i < treeView->model()->columnCount(); i++) {
      treeView->resizeColumnToContents(i);
    }
  } else {
    tabWidget->removeTab(static_cast<int>(ETabIdx::eGUI));
  }
}

const ::google::protobuf::Message& CMsgEditorDialog::getMessage() {
  return *m_pMessage;
}

void CMsgEditorDialog::handleCustomContextMenuRequested(const QPoint& pos) {
  QModelIndex index = treeView->indexAt(pos);
  if (!index.isValid()) {
    return;
  }

  index = index.siblingAtColumn(0);

  ProtoTreeModel* model = static_cast<ProtoTreeModel*>(treeView->model());
  const ProtoTreeItem* item = model->item(index);
  auto itemType = (item->data(ProtoTreeModel::kColumnTypeType, Qt::DisplayRole).toString());

  auto parentIndex = model->parent(index);
  const ProtoTreeItem* parentItem = model->item(parentIndex);

  auto parentType = (parentItem->data(ProtoTreeModel::kColumnTypeType, Qt::DisplayRole).toString());

  if (parentType != "repeated"
      && itemType != "repeated") {
      return;
  }

  QMenu* popMenu = new QMenu(this);
  if (itemType == "repeated") {
    QAction* pAdd = new QAction(tr("Add"), this);
    popMenu->addAction(pAdd);
    connect(pAdd, &QAction::triggered, this, [=]() {
      if (!model->insertModelData(m_pMessage->GetDescriptor(), index, item->childCount())) {
        QMessageBox::warning(nullptr, "Opt error", "Add item failed!");
      }
    });
    
    QAction* pClear = new QAction(tr("Clear"), this);
    popMenu->addAction(pClear);
    connect(pClear, &QAction::triggered, this, [=]() {
      if (QMessageBox::No == QMessageBox::question(this, "Warning", "Are you sure to clear it?")) {
        return;
      }

      if (!model->removeRows(0, item->childCount(), index)) {
        QMessageBox::warning(this, "Opt error", "Clear item failed!");
      }
    });
  }

  if (parentType == "repeated") {
    QAction* pInsert = new QAction(tr("Insert"), this);
    popMenu->addAction(pInsert);
    connect(pInsert, &QAction::triggered, this, [=]() {
      if (!model->insertModelData(m_pMessage->GetDescriptor(), parentIndex, index.row())) {
        QMessageBox::warning(this, "Opt error", "Insert item failed!");
      }
    });

    QAction* pDuplicate = new QAction(tr("Duplicate"), this);
    popMenu->addAction(pDuplicate);
    connect(pDuplicate, &QAction::triggered, this, [=]() {
      if (!model->duplicateModelData(parentIndex, index.row())) {
        QMessageBox::warning(this, "Opt error", "Duplicate item failed!");
      }
            });

    QAction* pDel = new QAction(tr("Delete"), this);
    popMenu->addAction(pDel);
    connect(pDel, &QAction::triggered, this, [=]() {
      if (!model->removeModelData(parentIndex, index.row())) {
        QMessageBox::warning(this, "Opt error", "Remove item failed!");
      }
            });
  }

  popMenu->exec(QCursor::pos());
  delete popMenu;
}

void CMsgEditorDialog::handleTreeViewExpanded(const QModelIndex& index) {
  treeView->resizeColumnToContents(0);
}

void CMsgEditorDialog::handleJsonBrowseBtnClicked() {
  QString jsonFileName = QFileDialog::getOpenFileName(this,
                                                      tr("Open json file"),
                                                      QString(),
                                                      "Json (*.json *txt)");

  if (jsonFileName.isEmpty()) {
    return;
  }

  QFile jsonFile(jsonFileName);
  jsonFile.open(QIODevice::ReadOnly | QIODevice::Text);
  if (!jsonFile.isOpen()) {
    QMessageBox::warning(nullptr, "Open error", "cannot open" + jsonFileName);
    return;
  }

  if (!jsonFile.isReadable()) {
    QMessageBox::warning(nullptr, "Open error", "cannot read" + jsonFileName);
    return;
  }

  textEdit->setText(jsonFile.readAll());
}

void CMsgEditorDialog::handleJsonParseBtnClicked() {
  auto status = google::protobuf::util::JsonStringToMessage(textEdit->toPlainText().toStdString(), m_pMessage);
  if (status.ok()) {
    btnParse->setText("Parsed");
    btnParse->setDisabled(true);
    okButton->setDisabled(false);
    okButton->setDefault(true);
    // 刷新
    if (!m_isAnyType) {
      updateGUIData();
    }
  } else {
    QMessageBox::warning(nullptr, "Parse error", status.error_message().data());
  }
}

void CMsgEditorDialog::handleTabBarChanged(int index) {
  // 切换到json编辑分页
  if (static_cast<int>(ETabIdx::eJSON) == index) {
    auto* model = static_cast<ProtoTreeModel*>(treeView->model());
    m_pMessage->Clear();
    model->getMessage(*m_pMessage);
    std::string msgStr;
    auto status = google::protobuf::util::MessageToJsonString(*m_pMessage, &msgStr, ConfigHelper::instance().getJsonPrintOption());

    if (status.ok()) {
      textEdit->setText(msgStr.c_str());
      btnParse->setText("Parsed");
      btnParse->setDisabled(true);
    } else {
      QMessageBox::warning(nullptr, "Serialize error", status.error_message().data());
    }
  }
  okButton->setDisabled(false);
}

void CMsgEditorDialog::handleTextEditTextChange() {
  btnParse->setText("Parse");
  btnParse->setDisabled(false);
  btnParse->setDefault(true);
  okButton->setDisabled(true);
}

void CMsgEditorDialog::updateGUIData() {
  QAbstractItemModel* oldModel = treeView->model();
  if (oldModel != nullptr) {
    oldModel->disconnect(SIGNAL(dataChanged(QModelIndex, QModelIndex)));
  }
  auto* model = new ProtoTreeModel(*m_pMessage);
  QItemSelectionModel* selectionModel = treeView->selectionModel();
  connect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(treeViewDataChanged()));
  treeView->setItemDelegate(&m_delegate);
  treeView->setModel(model);
  treeView->reset();
  delete selectionModel;
  delete oldModel;

  treeView->expandAll();
  for (int i = 1; i < treeView->model()->columnCount(); i++) {
    treeView->resizeColumnToContents(i);
  }
}

void CMsgEditorDialog::handleApplyButtonClicked() {
  if (tabWidget->currentIndex() != static_cast<int>(ETabIdx::eGUI)) {
    if (btnParse->isEnabled()) {
      handleJsonParseBtnClicked();
    }

    if (btnParse->isEnabled()) {
      return;
    }
  } else {
    auto* model = static_cast<ProtoTreeModel*>(treeView->model());
    m_pMessage->Clear();
    model->getMessage(*m_pMessage);
  }

  accept();
}

void CMsgEditorDialog::treeViewDataChanged() {
  for (int i = 0; i < treeView->model()->columnCount(); i++) {
    treeView->resizeColumnToContents(i);
  }
}

QWidget* QtTreeViewItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
  QVariant value = index.data(Qt::DisplayRole);
  if (value.isValid()) {
    auto type = static_cast<google::protobuf::FieldDescriptor::CppType>(index.data(Qt::UserRole).toInt());
    switch (type) {
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      {
        QLineEdit* editor = new QLineEdit(parent);
        editor->setText(value.toString());
        return editor;
      }
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
      {
        QComboBox* editor = new QComboBox(parent);
        editor->view()->setItemDelegate(new PopupItemDelegate);

        auto enumNames = index.data(Qt::UserRole + 1).toStringList();

        for (auto& enumName : enumNames) {
          editor->addItem(enumName);
        }

        editor->setCurrentText(value.toString());
        return editor;
      }
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
      return nullptr;
    default:
      break;
    }
  }
  return QStyledItemDelegate::createEditor(parent, option, index);
}

void QtTreeViewItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
  QVariant value = index.data(Qt::DisplayRole);
  if (value.isValid()) {
    auto type = static_cast<google::protobuf::FieldDescriptor::CppType>(index.data(Qt::UserRole).toInt());
    switch (type) {
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      {
        auto lineEditor = qobject_cast<QLineEdit*>(editor);
        if (lineEditor) {
          QVariant editorValue = lineEditor->text();
          bool ok;
          double val = editorValue.toDouble(&ok);
          if (ok) {
            model->setData(index, QVariant(val), Qt::EditRole);
          }
          return;
        }
      }
      break;
    default:
      break;
    }
  }
  QStyledItemDelegate::setModelData(editor, model, index);
}

void QtTreeViewItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
  QVariant value = index.data(Qt::DisplayRole);
  if (value.isValid()) {
    auto type = static_cast<google::protobuf::FieldDescriptor::CppType>(index.data(Qt::UserRole).toInt());
    switch (type) {
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
      {
        bool checked = value.toBool();
        QStyleOptionButton buttonOption;
        buttonOption.state |= QStyle::State_Active; // Required!
        buttonOption.state |= ((index.flags() & Qt::ItemIsEditable) ? QStyle::State_Enabled : QStyle::State_ReadOnly);
        buttonOption.state |= (checked ? QStyle::State_On : QStyle::State_Off);
        QRect checkBoxRect = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &buttonOption); // Only used to get size of native checkbox widget.
        buttonOption.rect = QStyle::alignedRect(option.direction, Qt::AlignLeft | Qt::AlignVCenter, checkBoxRect.size(), option.rect); // Our checkbox rect.
        buttonOption.rect.adjust(3, 0, 0, 0);
        buttonOption.palette.setColor(QPalette::Window, Qt::white);
        QApplication::style()->drawControl(QStyle::CE_CheckBox, &buttonOption, painter);
        return;
      }
    default:
      break;
    }
  }
  QStyledItemDelegate::paint(painter, option, index);
}

QSize QtTreeViewItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
  QSize s = QStyledItemDelegate::sizeHint(option, index);
  s.setHeight(20);
  return s;
}

bool QtTreeViewItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
  if (event->type() == QEvent::MouseButtonRelease) {
    if (m_lastClickedIndex != index) {
      m_lastClickedIndex = index;
      return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
  }

  QVariant value = index.data(Qt::DisplayRole);
  if (value.isValid()) {

    auto type = static_cast<google::protobuf::FieldDescriptor::CppType>(index.data(Qt::UserRole).toInt());
    if (type == google::protobuf::FieldDescriptor::CPPTYPE_BOOL) {
      if (event->type() == QEvent::MouseButtonDblClick) {
        return false;
      }

      if (event->type() != QEvent::MouseButtonRelease) {
        return false;
      }

      if (m_lastClickedIndex != index) {
        return false;
      }

      auto* mouseEvent = static_cast<QMouseEvent*>(event);
      if (mouseEvent->button() != Qt::LeftButton) {
        return false;
      }

      if (!option.rect.contains(mouseEvent->pos())) {
        return false;
      }

      bool checked = value.toBool();
      bool success = model->setData(index, !checked, Qt::EditRole);

      if (success) {
        model->dataChanged(index.sibling(index.row(), 0), index.sibling(index.row(), model->columnCount()));
      }
      return success;
    }
  }
  return QStyledItemDelegate::editorEvent(event, model, option, index);
}
