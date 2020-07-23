#include "SettingDialog.h"
#include <QFileDialog>
#include <QComboBox>
#include <QStyledItemDelegate>


#include "ConfigHelper.h"
#include "LineEdit.h"

CSettingDialog::CSettingDialog(QWidget *parent)
    : QDialog(parent), Ui_SettingDlg() {
    setupUi(this);
    QObject::connect(btnProtoPath, &QPushButton::clicked, this, &CSettingDialog::handleSelectProtoBtnClicked);
    QObject::connect(btnScript, &QPushButton::clicked, this, &CSettingDialog::handleSelectScriptBtnClicked);
    QObject::connect(okButton, &QPushButton::clicked, this, &CSettingDialog::handleOkBtnClicked);
    QObject::connect(cbProtoPath, SIGNAL(currentTextChanged(const QString&)), this, SLOT(handlePathChanged(const QString&)));
    QObject::connect(cbScriptPath, SIGNAL(currentTextChanged(const QString&)), this, SLOT(handlePathChanged(const QString&)));
}

CSettingDialog::~CSettingDialog()
{
}

void CSettingDialog::init(bool bFirstOpen) {
    {
        auto* pLineEdit = new CLineEdit(cbProtoPath);
        cbProtoPath->setLineEdit(pLineEdit);
    }
    cbProtoPath->setItemDelegate(new QStyledItemDelegate(cbProtoPath));
    cbProtoPath->view()->installEventFilter(new DeleteHighlightedItemFilter(cbProtoPath));
    ConfigHelper::instance().restoreWidgetComboxState("ProtoPath", *cbProtoPath);

    {
        auto* pLineEdit = new CLineEdit(cbScriptPath);
        cbScriptPath->setLineEdit(pLineEdit);
    }
    cbScriptPath->setItemDelegate(new QStyledItemDelegate(cbScriptPath));
    cbScriptPath->view()->installEventFilter(new DeleteHighlightedItemFilter(cbScriptPath));
    ConfigHelper::instance().restoreWidgetComboxState("LuaScriptPath", *cbScriptPath);

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
        ConfigHelper::instance().saveProtoPath(fileDialog->selectedFiles()[0]);
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
        ConfigHelper::instance().saveLuaScriptPath(fileDialog->selectedFiles()[0]);
    }
}

void CSettingDialog::handlePathChanged(const QString& newText) {
    checkAvailable();
}

void CSettingDialog::handleOkBtnClicked() {
    if (!cbProtoPath->lineEdit()->text().isEmpty()) {
        addNewItemIntoCombox(*cbProtoPath);
        ConfigHelper::instance().saveWidgetComboxState("ProtoPath", *cbProtoPath);
    }

    if (!cbScriptPath->lineEdit()->text().isEmpty()) {
        addNewItemIntoCombox(*cbScriptPath);
        ConfigHelper::instance().saveWidgetComboxState("LuaScriptPath", *cbScriptPath);
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

void CSettingDialog::addNewItemIntoCombox(QComboBox& combox) {
    int cbIdx = combox.findText(combox.currentText());
    if (-1 == cbIdx) {
        combox.insertItem(0, combox.currentText());
        if (combox.count() > ConfigHelper::instance().getHistroyComboxItemMaxCnt()) {
            combox.removeItem(combox.count() - 1);
        }
    } else {
        QString text = combox.currentText();
        combox.removeItem(cbIdx);
        combox.insertItem(0, text);
    }
    combox.setCurrentIndex(0);
}
