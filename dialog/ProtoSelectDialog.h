#pragma once

#include <QDialog>
#include "ui_ProtoPathSelect.h"

class CProtoSelectDialog : public QDialog, public Ui_ProtoPathDlg
{
    Q_OBJECT

public:
    CProtoSelectDialog(QWidget *parent);
    ~CProtoSelectDialog();

    void init(const QString& rootPath, const QString& loadPath);

    QString getRootPath();
    QString getLoadPath();

public slots:
    void handleSelectRootBtnClicked();
    void handleSelectLoadBtnClicked();

private:
};
