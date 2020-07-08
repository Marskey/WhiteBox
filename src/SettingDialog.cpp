#include "SettingDialog.h"
#include <QFileDialog>

#include "ConfigHelper.h"

CSettingDialog::CSettingDialog(QWidget *parent)
    : QDialog(parent), Ui_SettingDlg() {
    setupUi(this);
    QObject::connect(btnProtoPath, &QPushButton::clicked, this, &CSettingDialog::handleSelectProtoBtnClicked);
    QObject::connect(btnScript, &QPushButton::clicked, this, &CSettingDialog::handleSelectScriptBtnClicked);
    QObject::connect(editProtoPath, SIGNAL(textChanged(const QString&)), this, SLOT(handlePathChanged(const QString&)));
    QObject::connect(editScriptPath, SIGNAL(textChanged(const QString&)), this, SLOT(handlePathChanged(const QString&)));
}

CSettingDialog::~CSettingDialog()
{
}

void CSettingDialog::init(bool bFirstOpen) {
    editProtoPath->setText(ConfigHelper::instance().getProtoPath());
    editScriptPath->setText(ConfigHelper::instance().getLuaScriptPath());

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
        editProtoPath->setText(fileDialog->selectedFiles()[0]);
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
        editScriptPath->setText(fileDialog->selectedFiles()[0]);
        ConfigHelper::instance().saveLuaScriptPath(fileDialog->selectedFiles()[0]);
    }
}

void CSettingDialog::handlePathChanged(const QString& newText) {
    if (!editProtoPath->text().isEmpty()
        && !editScriptPath->text().isEmpty()) {
        ConfigHelper::instance().saveProtoPath(editProtoPath->text());
        ConfigHelper::instance().saveLuaScriptPath(editScriptPath->text());
    }

    checkAvailable();
}

void CSettingDialog::checkAvailable() {
    if (!editProtoPath->text().isEmpty()
        && !editScriptPath->text().isEmpty()) {
        okButton->setEnabled(true);
    } else {
        okButton->setEnabled(false);
    }
}
