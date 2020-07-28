#pragma once

#include "ui_MessageEditor.h"

#include "google/protobuf/message.h"
#include <google/protobuf/util/json_util.h>


class CJsonHighlighter;
class CLineEdit;
class QFormLayout;
class CMsgEditorDialog : public QDialog, public Ui_MessageEditor
{
    Q_OBJECT

public:
    CMsgEditorDialog(QWidget *parent = 0);
    ~CMsgEditorDialog();

    void initDialogByMessage(const ::google::protobuf::Message* pMessage);
    const ::google::protobuf::Message& getMessage();

private:
    void createWidget(QFormLayout& layout, std::string strFiledName, const google::protobuf::FieldDescriptor* pDescriptor);

    void registerBtn(QPushButton* pBtn, int index);
    int getBtnRegisterIndex(QPushButton* pBtn);
    void updateGUIData();

private slots:
    void handleComboxIndexChanged(int index);

    void handleLineEdit(const QString& text);

    void handleMessageEdit();

    void handleMessageAddBtn();
    void handleMessageInsertBtn();
    void handleValueAddBtn();
    void handleValueInsertBtn();

    void handleRemoveBtn();
    void handleClearBtn();

    void handleJsonBrowseBtnClicked();
    void handleJsonParseBtnClicked();
    void handleTabBarClicked(int index);

    void handleTextEditTextChange();
private:
    ::google::protobuf::Message* m_pMessage = nullptr;
    std::map<QPushButton*, int> m_mapBtn2MsgIdx;
    CJsonHighlighter* m_highlighter;
};
