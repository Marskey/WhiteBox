#include "ProtoSelectDialog.h"
#include <QFileDialog>

CProtoSelectDialog::CProtoSelectDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    QObject::connect(btnLoadPath, &QPushButton::clicked, this, &CProtoSelectDialog::handleSelectLoadBtnClicked);
    QObject::connect(btnRootPath, &QPushButton::clicked, this, &CProtoSelectDialog::handleSelectRootBtnClicked);
}

CProtoSelectDialog::~CProtoSelectDialog()
{
}

void CProtoSelectDialog::init(const QString& rootPath, const QString& loadPath) {
    editRootPath->setText(rootPath);
    editLoadPath->setText(loadPath);

    if (!editRootPath->text().isEmpty()
        && !editLoadPath->text().isEmpty()) {
        okButton->setEnabled(true);
    }
}

QString CProtoSelectDialog::getRootPath() {
    return editRootPath->text();
}

QString CProtoSelectDialog::getLoadPath() {
    return editLoadPath->text();
}

void CProtoSelectDialog::handleSelectRootBtnClicked() {
    QFileDialog* fileDialog = new QFileDialog(this);
    fileDialog->setWindowTitle("Select ProtoRootPath");
    fileDialog->setFileMode(QFileDialog::DirectoryOnly);
    fileDialog->setViewMode(QFileDialog::Detail);
    if (fileDialog->exec()) {
        editRootPath->setText(fileDialog->selectedFiles()[0]);
    }

    if (!editRootPath->text().isEmpty()
        && !editLoadPath->text().isEmpty()) {
        okButton->setEnabled(true);
    }
}

void CProtoSelectDialog::handleSelectLoadBtnClicked() {
    QFileDialog* fileDialog = new QFileDialog(this);
    fileDialog->setWindowTitle("Select ProtoPath");
    fileDialog->setFileMode(QFileDialog::DirectoryOnly);
    fileDialog->setViewMode(QFileDialog::Detail);
    if (fileDialog->exec()) {
        editLoadPath->setText(fileDialog->selectedFiles()[0]);
    }

    if (!editRootPath->text().isEmpty()
        && !editLoadPath->text().isEmpty()) {
        okButton->setEnabled(true);
    }
}
