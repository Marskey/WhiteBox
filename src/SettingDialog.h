#pragma once

#include "ui_Setting.h"

class CSettingDialog : public QDialog, public Ui_SettingDlg
{
    Q_OBJECT

public:
    CSettingDialog(QWidget *parent);
    ~CSettingDialog();

    void init(bool bFirstOpen);

public slots:
    void handleSelectProtoBtnClicked();
    void handleSelectScriptBtnClicked();
    void handlePathChanged(const QString& newText);

private:
    void checkAvailable();
};
