#include "MsgEditorDialog.h"
#include "LineEdit.h"
#include "JsonHighlighter.h"

#include <QFormLayout>
#include <QListWidget>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <qpainter.h>
#include <QTextEdit>


#include "ConfigHelper.h"
#include "google/protobuf/repeated_field.h"
#include "ProtoManager.h"
#include "ProtoTreeModel.h"

//#569ad6

class ProtoTreeModel;

CMsgEditorDialog::CMsgEditorDialog(QWidget *parent)
    : QDialog(parent, Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint) {
    setupUi(this);

    connect(btnBrowse, SIGNAL(clicked()), this, SLOT(handleJsonBrowseBtnClicked()));
    connect(btnParse, SIGNAL(clicked()), this, SLOT(handleJsonParseBtnClicked()));
    connect(tabWidget, SIGNAL(tabBarClicked(int)), this, SLOT(handleTabBarClicked(int)));
    connect(textEdit, SIGNAL(textChanged()), this, SLOT(handleTextEditTextChange()));
    connect(okButton, SIGNAL(clicked()), this, SLOT(handleApplyButtonClicked()));

    m_highlighter = new CJsonHighlighter(textEdit->document());
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
      for (int i = 0; i < treeView->model()->columnCount(); i++) {
        treeView->resizeColumnToContents(i);
      }
    } else {
        tabWidget->removeTab(static_cast<int>(ETabIdx::eGUI));
    }
}

const ::google::protobuf::Message& CMsgEditorDialog::getMessage() {
    return *m_pMessage;
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
        // 刷新
        if (!m_isAnyType) {
            updateGUIData();
        }
    } else {
        QMessageBox::warning(nullptr, "Parse error", status.error_message().data());
    }
}

void CMsgEditorDialog::handleTabBarClicked(int index) {
    // 切换到json编辑分页
    if (static_cast<int>(ETabIdx::eJSON) == index) {
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
}

void CMsgEditorDialog::handleTextEditTextChange() {
    btnParse->setText("Parse");
    btnParse->setDisabled(false);
    btnParse->setDefault(true);
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
  for (int i = 0; i < treeView->model()->columnCount(); i++) {
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
    }

//    auto* model = static_cast<ProtoTreeModel*>(treeView->model());
//    model->getMessage(*m_pMessage);

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

void QtTreeViewItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
  QStyledItemDelegate::setEditorData(editor, index);
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
        buttonOption.rect = QStyle::alignedRect(option.direction, Qt::AlignLeft, checkBoxRect.size(), option.rect); // Our checkbox rect.
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
