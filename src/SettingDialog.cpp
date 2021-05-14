#include "SettingDialog.h"
#include <QFileDialog>
#include <QComboBox>
#include <QStyledItemDelegate>


#include "ConfigHelper.h"
#include "LineEdit.h"

CSettingDialog::CSettingDialog(QWidget* parent)
  : QDialog(parent), Ui_SettingDlg() {
  setupUi(this);
  QObject::connect(btnProtoPath, &QPushButton::clicked, this, &CSettingDialog::handleSelectProtoBtnClicked);
  QObject::connect(btnScript, &QPushButton::clicked, this, &CSettingDialog::handleSelectScriptBtnClicked);
  QObject::connect(okButton, &QPushButton::clicked, this, &CSettingDialog::handleOkBtnClicked);
  QObject::connect(cbProtoPath, SIGNAL(currentTextChanged(const QString&)), this, SLOT(handlePathChanged(const QString&)));
  QObject::connect(cbScriptPath, SIGNAL(currentTextChanged(const QString&)), this, SLOT(handlePathChanged(const QString&)));
}

CSettingDialog::~CSettingDialog() {
}

void CSettingDialog::init(bool bFirstOpen) {
  {
    auto* pLineEdit = new CLineEdit(cbProtoPath);
    cbProtoPath->setLineEdit(pLineEdit);
  }
  cbProtoPath->view()->installEventFilter(new DeleteHighlightedItemFilter(cbProtoPath));
  ConfigHelper::instance().restoreWidgetComboBoxState("ProtoPath", *cbProtoPath);

  {
    auto* pLineEdit = new CLineEdit(cbScriptPath);
    cbScriptPath->setLineEdit(pLineEdit);
  }
  cbScriptPath->view()->installEventFilter(new DeleteHighlightedItemFilter(cbScriptPath));
  ConfigHelper::instance().restoreWidgetComboBoxState("LuaScriptPath", *cbScriptPath);

  checkAvailable();
  if (bFirstOpen) {
    okButton->setText("Ok");
  } else {
    okButton->setText("Reload");
  }
}

void CSettingDialog::handleSelectProtoBtnClicked() {
  auto* fileDialog = new QFileDialog(this);
  fileDialog->setWindowTitle("Select Proto Root Path");
  //fileDialog->setDirectory(QDir::currentPath());
  //fileDialog->setOption(QFileDialog::ShowDirsOnly);
  fileDialog->setFileMode(QFileDialog::DirectoryOnly);
  fileDialog->setViewMode(QFileDialog::Detail);
  if (fileDialog->exec()) {
    cbProtoPath->lineEdit()->setText(fileDialog->selectedFiles()[0]);
  }

  checkAvailable();
}

void CSettingDialog::handleSelectScriptBtnClicked() {
  auto* fileDialog = new QFileDialog(this);
  fileDialog->setWindowTitle("Select Lua Script Path");
  //fileDialog->setDirectory(QDir::currentPath());
  //fileDialog->setOption(QFileDialog::ShowDirsOnly);
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setViewMode(QFileDialog::Detail);
  if (fileDialog->exec()) {
    cbScriptPath->lineEdit()->setText(fileDialog->selectedFiles()[0]);
  }
}

void CSettingDialog::handlePathChanged(const QString& newText) {
  checkAvailable();
}

void CSettingDialog::handleOkBtnClicked() {
  if (!cbProtoPath->lineEdit()->text().isEmpty()) {
    addNewItemIntoComboBox(*cbProtoPath);
    ConfigHelper::instance().saveWidgetComboBoxState("ProtoPath", *cbProtoPath);
  }

  if (!cbScriptPath->lineEdit()->text().isEmpty()) {
    addNewItemIntoComboBox(*cbScriptPath);
    ConfigHelper::instance().saveWidgetComboBoxState("LuaScriptPath", *cbScriptPath);
  }
}

void CSettingDialog::checkAvailable() {
  if (!cbProtoPath->lineEdit()->text().isEmpty()
      && !cbScriptPath->lineEdit()->text().isEmpty()) {
    okButton->setEnabled(true);
  } else {
    okButton->setEnabled(false);
  }
}

void CSettingDialog::addNewItemIntoComboBox(QComboBox& comboBox) {
  int cbIdx = comboBox.findText(comboBox.currentText());
  if (-1 == cbIdx) {
    comboBox.insertItem(0, comboBox.currentText());
    if (comboBox.count() > ConfigHelper::instance().getHistroyComboBoxItemMaxCnt()) {
      comboBox.removeItem(comboBox.count() - 1);
    }
  } else {
    QString text = comboBox.currentText();
    comboBox.removeItem(cbIdx);
    comboBox.insertItem(0, text);
  }
  comboBox.setCurrentIndex(0);
}
