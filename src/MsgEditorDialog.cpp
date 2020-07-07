#include "MsgEditorDialog.h"
#include "LineEdit.h"

#include <QFormLayout>
#include <QListWidget>
#include <QComboBox>
#include <QMessageBox>
#include <QTextEdit>

#include "google/protobuf/repeated_field.h"
#include "ProtoManager.h"

CMsgEditorDialog::CMsgEditorDialog(QWidget *parent)
    : QDialog(parent) {
    setupUi(this);

    m_option.always_print_primitive_fields = true;
    m_option.preserve_proto_field_names = true;
}

CMsgEditorDialog::~CMsgEditorDialog() {
    delete m_pMessage;
    m_pMessage = nullptr;
}

void CMsgEditorDialog::initDialogByMessage(const ::google::protobuf::Message* pMessage) {
    m_pMessage = pMessage->New();
    m_pMessage->CopyFrom(*pMessage);

    // 设置标题
    labelTitle->setText(m_pMessage->GetTypeName().c_str());

    auto* scrollAreaContent = new QWidget(this);
    auto* pFormLayout = new QFormLayout(scrollAreaContent);

    int cnt = pMessage->GetDescriptor()->field_count();
    for (int i = 0; i < cnt; ++i) {
        const google::protobuf::FieldDescriptor* pFd = pMessage->GetDescriptor()->field(i);
        createWidget(*pFormLayout, pFd->name(), pFd);
    }

    scrollAreaContent->setLayout(pFormLayout);
    scrollArea->setWidget(scrollAreaContent);
}

const ::google::protobuf::Message& CMsgEditorDialog::getMessage() {
    return *m_pMessage;
}

void CMsgEditorDialog::createWidget(QFormLayout& layout, std::string strFiledName, const google::protobuf::FieldDescriptor* pDescriptor) {
    if (nullptr == pDescriptor) {
        return;
    }

    auto* pFieldLabel = new QLabel(fmt::format("<h6>{1}</h6> <span style=\"color: #569ad6\">{0}</span>"
                                               , pDescriptor->cpp_type_name()
                                               , strFiledName).c_str()
                                   , this);

    if (pDescriptor->is_repeated()) {
        if (pDescriptor->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
            // repeated messsage
            auto* pBtn1 = new QPushButton("Add", this);
            registerBtn(pBtn1, pDescriptor->number());
            connect(pBtn1, SIGNAL(clicked()), this, SLOT(handleMessageAddBtn()));

            auto* pBtn2 = new QPushButton("Insert", this);
            registerBtn(pBtn2, pDescriptor->number());
            connect(pBtn2, SIGNAL(clicked()), this, SLOT(handleMessageInsertBtn()));

            auto* pBtn3 = new QPushButton("Remove", this);
            registerBtn(pBtn3, pDescriptor->number());
            connect(pBtn3, SIGNAL(clicked()), this, SLOT(handleRemoveBtn()));

            auto* pBtn4 = new QPushButton("Clear", this);
            registerBtn(pBtn4, pDescriptor->number());
            connect(pBtn4, SIGNAL(clicked()), this, SLOT(handleClearBtn()));

            auto* pHBoxLayout = new QHBoxLayout(this);
            pHBoxLayout->addWidget(pBtn1);
            pHBoxLayout->addWidget(pBtn2);
            pHBoxLayout->addWidget(pBtn3);
            pHBoxLayout->addWidget(pBtn4);

            auto* pListWidget = new QListWidget(this);
            std::string listName = "list_" + std::to_string(pDescriptor->number());
            pListWidget->setObjectName(listName.c_str());
            int size = m_pMessage->GetReflection()->FieldSize(*m_pMessage, pDescriptor);
            for (int i = 0; i < size; ++i) {
                pListWidget->addItem(ProtoManager::instance().getMsgValue(*m_pMessage, pDescriptor, i).c_str());
            }

            auto* pVBoxLayout = new QVBoxLayout(this);
            pVBoxLayout->addLayout(pHBoxLayout);
            pVBoxLayout->addWidget(pListWidget);

            layout.addRow(pFieldLabel, pVBoxLayout);
        } else {
            // repeated input
            auto* pHBoxLayout = new QHBoxLayout(this);
            if (pDescriptor->type() == google::protobuf::FieldDescriptor::TYPE_ENUM) {
                const google::protobuf::EnumDescriptor* pEd = pDescriptor->enum_type();

                // enum select
                auto* pComboBox = new QComboBox(this);
                pComboBox->setObjectName(("cb_" + std::to_string(pDescriptor->number())).c_str());

                for (int i = 0; i < pEd->value_count(); ++i) {
                    const google::protobuf::EnumValueDescriptor* value = pEd->value(i);
                    std::string enumName = value->name();

                    pComboBox->addItem(enumName.c_str());
                    pComboBox->setCurrentIndex(0);
                }
                pHBoxLayout->addWidget(pComboBox);
                
            } else if (pDescriptor->type() == google::protobuf::FieldDescriptor::TYPE_BOOL) {
                auto* pComboBox = new QComboBox(this);
                pComboBox->setObjectName(("cb_" + std::to_string(pDescriptor->number())).c_str());
                pComboBox->addItem("false", 0);
                pComboBox->addItem("true", 1);
                pHBoxLayout->addWidget(pComboBox);
            } else {
                auto* pLineEdit = new CLineEdit(this);
                std::string lineEditName = "lineEdit_" + std::to_string(pDescriptor->number());
                pLineEdit->setObjectName(lineEditName.c_str());
                pHBoxLayout->addWidget(pLineEdit);
            }

            auto* pBtn1 = new QPushButton("Add", this);
            registerBtn(pBtn1, pDescriptor->number());
            pHBoxLayout->addWidget(pBtn1);
            connect(pBtn1, SIGNAL(clicked()), this, SLOT(handleValueAddBtn()));

            auto* pBtn2 = new QPushButton("Insert", this);
            registerBtn(pBtn2, pDescriptor->number());
            pHBoxLayout->addWidget(pBtn2);
            connect(pBtn2, SIGNAL(clicked()), this, SLOT(handleValueInsertBtn()));

            auto* pBtn3 = new QPushButton("Remove", this);
            registerBtn(pBtn3, pDescriptor->number());
            pHBoxLayout->addWidget(pBtn3);
            connect(pBtn3, SIGNAL(clicked()), this, SLOT(handleRemoveBtn()));

            auto* pBtn4 = new QPushButton("Clear", this);
            registerBtn(pBtn4, pDescriptor->number());
            pHBoxLayout->addWidget(pBtn4);
            connect(pBtn4, SIGNAL(clicked()), this, SLOT(handleClearBtn()));

            auto* pListWidget = new QListWidget(this);
            std::string listName = "list_" + std::to_string(pDescriptor->number());
            pListWidget->setObjectName(listName.c_str());

            int size = m_pMessage->GetReflection()->FieldSize(*m_pMessage, pDescriptor);
            for (int i = 0; i < size; ++i) {
                pListWidget->addItem(ProtoManager::instance().getMsgValue(*m_pMessage, pDescriptor, i).c_str());
            }

            auto* pVBoxLayout = new QVBoxLayout(this);
            pVBoxLayout->addLayout(pHBoxLayout);
            pVBoxLayout->addWidget(pListWidget);

            layout.addRow(pFieldLabel, pVBoxLayout);
        }
    } else {
        google::protobuf::FieldDescriptor::Type filedType = pDescriptor->type();

        switch (filedType) {
            case google::protobuf::FieldDescriptor::TYPE_ENUM:
            {
                const google::protobuf::EnumDescriptor* pEd = pDescriptor->enum_type();

                int enumValue = m_pMessage->GetReflection()->GetEnumValue(*m_pMessage, pDescriptor);
                // enum select
                auto* pComboBox = new QComboBox(this);
                pComboBox->setObjectName(std::to_string(pDescriptor->number()).c_str());
                QObject::connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(handleComboxIndexChanged(int)));

                for (int i = 0; i < pEd->value_count(); ++i) {
                    const google::protobuf::EnumValueDescriptor* value = pEd->value(i);
                    std::string enumName = value->name();

                    pComboBox->addItem(enumName.c_str());
                    if (enumValue == value->number()) {
                        pComboBox->setCurrentIndex(i);
                    }
                }
                layout.addRow(pFieldLabel, pComboBox);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_INT32:
            case google::protobuf::FieldDescriptor::TYPE_INT64:
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
            case google::protobuf::FieldDescriptor::TYPE_UINT64:
            case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            {
                std::string value;
                if (filedType == google::protobuf::FieldDescriptor::TYPE_INT32) {
                    value = std::to_string(m_pMessage->GetReflection()->GetInt32(*m_pMessage, pDescriptor));
                } else if (filedType == google::protobuf::FieldDescriptor::TYPE_UINT32) {
                    value = std::to_string(m_pMessage->GetReflection()->GetUInt32(*m_pMessage, pDescriptor));
                } else if (filedType == google::protobuf::FieldDescriptor::TYPE_INT64) {
                    value = std::to_string(m_pMessage->GetReflection()->GetInt64(*m_pMessage, pDescriptor));
                } else if (filedType == google::protobuf::FieldDescriptor::TYPE_UINT64) {
                    value = std::to_string(m_pMessage->GetReflection()->GetUInt64(*m_pMessage, pDescriptor));
                } else if (filedType == google::protobuf::FieldDescriptor::TYPE_DOUBLE) {
                    value = std::to_string(m_pMessage->GetReflection()->GetDouble(*m_pMessage, pDescriptor));
                } else if (filedType == google::protobuf::FieldDescriptor::TYPE_FLOAT) {
                    value = std::to_string(m_pMessage->GetReflection()->GetFloat(*m_pMessage, pDescriptor));
                }

                // normal input
                auto* pLineEdit = new CLineEdit(this);
                pLineEdit->setObjectName(std::to_string(pDescriptor->number()).c_str());
                if (value.empty()) {
                    pLineEdit->setPlaceholderText("Enter value...");
                } else {
                    pLineEdit->setText(value.c_str());
                }
                QObject::connect(pLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(handleLineEdit(const QString&)));
                layout.addRow(pFieldLabel, pLineEdit);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_STRING:
            case google::protobuf::FieldDescriptor::TYPE_BYTES:
            {
                std::string value = m_pMessage->GetReflection()->GetString(*m_pMessage, pDescriptor);

                // normal input
                auto* pLineEdit = new CLineEdit(this);
                pLineEdit->setObjectName(std::to_string(pDescriptor->number()).c_str());
                if (value.empty()) {
                    pLineEdit->setPlaceholderText("Enter value...");
                } else {
                    pLineEdit->setText(value.c_str());
                }
                QObject::connect(pLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(handleLineEdit(const QString&)));
                layout.addRow(pFieldLabel, pLineEdit);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_BOOL:
            {
                bool value = m_pMessage->GetReflection()->GetBool(*m_pMessage, pDescriptor);
                auto* pComboBox = new QComboBox(this);
                pComboBox->addItem("false", 0);
                pComboBox->addItem("true", 1);
                pComboBox->setObjectName(std::to_string(pDescriptor->number()).c_str());
                if (value) {
                    pComboBox->setCurrentIndex(true);
                }
                QObject::connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(handleComboxIndexChanged(int)));
                layout.addRow(pFieldLabel, pComboBox);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_GROUP:
                break;
            case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
            {
                auto* pHBoxLayout = new QHBoxLayout(this);
                auto* pBtn1 = new QPushButton("Edit", this);
                registerBtn(pBtn1, pDescriptor->number());
                pHBoxLayout->addWidget(pBtn1);
                connect(pBtn1, SIGNAL(clicked()), this, SLOT(handleMessageEdit()));

                auto* pBtn2 = new QPushButton("Clear", this);
                registerBtn(pBtn2, pDescriptor->number());
                pHBoxLayout->addWidget(pBtn2);
                connect(pBtn2, SIGNAL(clicked()), this, SLOT(handleClearBtn()()));

                auto* pTextEdit = new QTextEdit(this);
                std::string textEditName = "textedit_" + std::to_string(pDescriptor->number());
                pTextEdit->setReadOnly(true);
                pTextEdit->setObjectName(textEditName.c_str());
                auto* pVBoxLayout = new QVBoxLayout(this);
                pVBoxLayout->addLayout(pHBoxLayout);
                pVBoxLayout->addWidget(pTextEdit);
                auto& message = m_pMessage->GetReflection()->GetMessage(*m_pMessage, pDescriptor);
                std::string msgStr;
                google::protobuf::util::MessageToJsonString(message, &msgStr, m_option);
                pTextEdit->setText(msgStr.c_str());

                layout.addRow(pFieldLabel, pVBoxLayout);
                break;
            }
            default:
                break;
        }
    }
}

void CMsgEditorDialog::handleComboxIndexChanged(int index) {
    auto* pCB = static_cast<QComboBox*>(sender());
    if (pCB->objectName().isEmpty()) {
        return;
    }

    int messageIdx = atoi(pCB->objectName().toStdString().c_str());

    const ::google::protobuf::FieldDescriptor* pFd = m_pMessage->GetDescriptor()->FindFieldByNumber(messageIdx);
    if (pFd->is_repeated()) {
        return;
    }

    google::protobuf::FieldDescriptor::Type filedType = pFd->type();

    if (google::protobuf::FieldDescriptor::TYPE_ENUM == filedType) {
        int enumIdx = pFd->enum_type()->FindValueByName(pCB->currentText().toStdString().c_str())->number();
        m_pMessage->GetReflection()->SetEnumValue(m_pMessage, pFd, enumIdx);
    } else if (google::protobuf::FieldDescriptor::TYPE_BOOL == filedType) {
        bool b = pCB->currentIndex();
        m_pMessage->GetReflection()->SetBool(m_pMessage, pFd, b);
    }
}

void CMsgEditorDialog::handleLineEdit(const QString& text) {
    auto* pLE = static_cast<CLineEdit*>(sender());
    if (pLE->objectName().isEmpty()) {
        return;
    }

    int messageIdx = atoi(pLE->objectName().toStdString().c_str());

    const ::google::protobuf::FieldDescriptor* pFd = m_pMessage->GetDescriptor()->FindFieldByNumber(messageIdx);
    if (pFd->is_repeated()) {
        return;
    }

    std::string value = pLE->text().toStdString();

    google::protobuf::FieldDescriptor::Type filedType = pFd->type();

    switch (filedType) {
    case google::protobuf::FieldDescriptor::TYPE_INT64:
        m_pMessage->GetReflection()->SetInt64(m_pMessage, pFd, _atoi64(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_INT32:
        m_pMessage->GetReflection()->SetInt32(m_pMessage, pFd, atoi(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_UINT32:
        m_pMessage->GetReflection()->SetUInt32(m_pMessage, pFd, atoi(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_UINT64:
        m_pMessage->GetReflection()->SetUInt64(m_pMessage, pFd, _atoi64(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
        m_pMessage->GetReflection()->SetDouble(m_pMessage, pFd, atof(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_FLOAT:
        m_pMessage->GetReflection()->SetFloat(m_pMessage, pFd, atof(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_STRING:
    case google::protobuf::FieldDescriptor::TYPE_BYTES:
        m_pMessage->GetReflection()->SetString(m_pMessage, pFd, value);
        break;
    }
}

void CMsgEditorDialog::handleMessageEdit() {
    auto* pBtn = static_cast<QPushButton*>(sender());
    int messageIdx = getBtnRegisterIndex(pBtn);

    const ::google::protobuf::FieldDescriptor* pFd = m_pMessage->GetDescriptor()->FindFieldByNumber(messageIdx);
    if (pFd->is_repeated()) {
        return;
    }

    google::protobuf::Message* pEditMessage = m_pMessage->GetReflection()->MutableMessage(m_pMessage, pFd);
    auto* pDlg = new CMsgEditorDialog;
    pDlg->initDialogByMessage(pEditMessage);
    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        pEditMessage->CopyFrom(pDlg->getMessage());
        if (pBtn->parentWidget() != nullptr) {
            if (pBtn->parentWidget()->layout() != nullptr) {
                std::string textEditName = "textedit_" + std::to_string(pFd->number());
                auto pTextEdit  = pBtn->parentWidget()->findChild<QTextEdit*>(textEditName.c_str());

                std::string msgStr;
                google::protobuf::util::MessageToJsonString(*pEditMessage, &msgStr, m_option);
                pTextEdit->setText(msgStr.c_str());
            }
        }
    }
    delete pDlg;
}

void CMsgEditorDialog::handleMessageAddBtn() {
    auto* pBtn = static_cast<QPushButton*>(sender());
    int messageIdx = getBtnRegisterIndex(pBtn);

    const ::google::protobuf::FieldDescriptor* pFd = m_pMessage->GetDescriptor()->FindFieldByNumber(messageIdx);
    if (!pFd->is_repeated()) {
        return;
    }

    google::protobuf::Message* pNewMessage = m_pMessage->GetReflection()->AddMessage(m_pMessage, pFd);
    auto* pDlg = new CMsgEditorDialog;
    pDlg->initDialogByMessage(pNewMessage);
    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        pNewMessage->CopyFrom(pDlg->getMessage());

        if (pBtn->parentWidget() != nullptr) {
            if (pBtn->parentWidget()->layout() != nullptr) {
                std::string listName = "list_" + std::to_string(messageIdx);
                QListWidget* pListWidget  = pBtn->parentWidget()->findChild<QListWidget*>(listName.c_str());

                std::string msgStr;
                google::protobuf::util::MessageToJsonString(*pNewMessage, &msgStr, m_option);
                pListWidget->addItem(msgStr.c_str());
            }
        }
    }
    delete pDlg;
}

void CMsgEditorDialog::handleMessageInsertBtn() {
    auto* pBtn = static_cast<QPushButton*>(sender());
    int messageIdx = getBtnRegisterIndex(pBtn);

    const ::google::protobuf::FieldDescriptor* pFd = m_pMessage->GetDescriptor()->FindFieldByNumber(messageIdx);
    if (!pFd->is_repeated()) {
        return;
    }

    google::protobuf::Message* pNewMessage = m_pMessage->GetReflection()->AddMessage(m_pMessage, pFd);
    auto* pDlg = new CMsgEditorDialog;
    pDlg->initDialogByMessage(pNewMessage);
    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        pNewMessage->CopyFrom(pDlg->getMessage());

        if (pBtn->parentWidget() != nullptr) {
            if (pBtn->parentWidget()->layout() != nullptr) {
                std::string listName = "list_" + std::to_string(messageIdx);
                auto* pListWidget  = pBtn->parentWidget()->findChild<QListWidget*>(listName.c_str());

                int curRow = pListWidget->currentRow();
                if (-1 == curRow) {
                    curRow = 0;
                }

                int count = m_pMessage->GetReflection()->FieldSize(*m_pMessage, pFd);
                for (int i = count - 1; i > curRow; --i) {
                    m_pMessage->GetReflection()->SwapElements(m_pMessage, pFd, i, i - 1);
                }

                std::string msgStr;
                google::protobuf::util::MessageToJsonString(*pNewMessage, &msgStr, m_option);
                pListWidget->insertItem(curRow, msgStr.c_str());
            }
        }
    }
    delete pDlg;
}

void CMsgEditorDialog::handleRemoveBtn() {
    auto* pBtn = static_cast<QPushButton*>(sender());
    int messageIdx = getBtnRegisterIndex(pBtn);

    const google::protobuf::FieldDescriptor* pFd = m_pMessage->GetDescriptor()->FindFieldByNumber(messageIdx);

    if (!pFd->is_repeated()) {
        return;        
    }

    if (pBtn->parentWidget() != nullptr) {
        if (pBtn->parentWidget()->layout() != nullptr) {
            std::string listName = "list_" + std::to_string(messageIdx);
            auto* pListWidget = pBtn->parentWidget()->findChild<QListWidget*>(listName.c_str());
            if (nullptr == pListWidget) {
                return;
            }

            int curRow = pListWidget->currentRow();
            if (-1 == curRow) {
                return;
            }

            int count = m_pMessage->GetReflection()->FieldSize(*m_pMessage, pFd);
            for (int i = curRow; i < count - 1; ++i) {
                m_pMessage->GetReflection()->SwapElements(m_pMessage, pFd, i, i + 1);
            }
            m_pMessage->GetReflection()->RemoveLast(m_pMessage, pFd);
            delete pListWidget->takeItem(curRow);
        }
    }
}

void CMsgEditorDialog::handleValueAddBtn() {
    auto* pBtn = static_cast<QPushButton*>(sender());
    int messageIdx = getBtnRegisterIndex(pBtn);

    const google::protobuf::FieldDescriptor* pfd = m_pMessage->GetDescriptor()->FindFieldByNumber(messageIdx);
    if (!pfd->is_repeated()) {
        return;        
    }

    std::string value = "";
    std::string textValue = "";
    QListWidget* pListWidget = nullptr;

    if (pBtn->parentWidget() != nullptr) {
        if (pBtn->parentWidget()->layout() != nullptr) {
            if (pfd->type() == google::protobuf::FieldDescriptor::TYPE_ENUM) {
                std::string cbName = "cb_" + std::to_string(messageIdx);
                auto* pCbEdit = pBtn->parentWidget()->findChild<QComboBox*>(cbName.c_str());
                if (pCbEdit != nullptr) {
                    value = std::to_string(pfd->enum_type()->FindValueByName(pCbEdit->currentText().toStdString().c_str())->number());
                    textValue = pCbEdit->currentText().toStdString();
                }
            } else if (pfd->type() == google::protobuf::FieldDescriptor::TYPE_BOOL) {
                std::string cbName = "cb_" + std::to_string(messageIdx);
                auto* pCbEdit = pBtn->parentWidget()->findChild<QComboBox*>(cbName.c_str());
                if (pCbEdit != nullptr) {
                    value = std::to_string(pCbEdit->currentIndex());
                    textValue = pCbEdit->currentText().toStdString();
                }
            } else {
                std::string lineEditName = "lineEdit_" + std::to_string(messageIdx);
                auto* pLineEdit = pBtn->parentWidget()->findChild<CLineEdit*>(lineEditName.c_str());
                if (pLineEdit != nullptr) {
                    value = pLineEdit->text().toStdString();
                    textValue = value;
                }
            }

            std::string listName = "list_" + std::to_string(messageIdx);
            pListWidget = pBtn->parentWidget()->findChild<QListWidget*>(listName.c_str());
        }
    }

    // 不能为空
    if (value.empty()) {
        return;
    }

    google::protobuf::FieldDescriptor::Type filedType = pfd->type();
    if (filedType != google::protobuf::FieldDescriptor::TYPE_STRING
        && filedType != google::protobuf::FieldDescriptor::TYPE_BYTES) {
        // check if is numberic
        for (auto i = 0; i < value.length(); ++i) {
            if (!isdigit(value[i])) {
                QMessageBox msgBox;
                msgBox.setText("You need enter digits.");
                msgBox.exec();
                return;
            }
        }
    }

    switch (filedType) {
    case google::protobuf::FieldDescriptor::TYPE_ENUM:
        m_pMessage->GetReflection()->AddEnumValue(m_pMessage, pfd, atoi(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_INT64:
        m_pMessage->GetReflection()->AddInt64(m_pMessage, pfd, atol(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_INT32:
        m_pMessage->GetReflection()->AddInt32(m_pMessage, pfd, atoi(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_UINT32:
        m_pMessage->GetReflection()->AddUInt32(m_pMessage, pfd, atoi(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_UINT64:
        m_pMessage->GetReflection()->AddUInt64(m_pMessage, pfd, atol(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
        m_pMessage->GetReflection()->AddDouble(m_pMessage, pfd, atol(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_FLOAT:
        m_pMessage->GetReflection()->AddFloat(m_pMessage, pfd, atof(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_STRING:
    case google::protobuf::FieldDescriptor::TYPE_BYTES:
        m_pMessage->GetReflection()->AddString(m_pMessage, pfd, value);
        break;
    case google::protobuf::FieldDescriptor::TYPE_BOOL:
        m_pMessage->GetReflection()->AddBool(m_pMessage, pfd, bool(atoi(value.c_str()) != 0));
        break;
    default:
        break;
    }

    if (nullptr != pListWidget) {
        pListWidget->addItem(textValue.c_str());
    }
}

void CMsgEditorDialog::handleValueInsertBtn() {
    auto* pBtn = static_cast<QPushButton*>(sender());
    int messageIdx = getBtnRegisterIndex(pBtn);

    const google::protobuf::FieldDescriptor* pfd = m_pMessage->GetDescriptor()->FindFieldByNumber(messageIdx);
    if (!pfd->is_repeated()) {
        return;        
    }

    std::string value = "";
    std::string textValue = "";
    QListWidget* pListWidget = nullptr;

    if (pBtn->parentWidget() != nullptr) {
        if (pBtn->parentWidget()->layout() != nullptr) {
            if (pfd->type() == google::protobuf::FieldDescriptor::TYPE_ENUM) {
                std::string cbName = "cb_" + std::to_string(messageIdx);
                auto* pCbEdit = pBtn->parentWidget()->findChild<QComboBox*>(cbName.c_str());
                if (pCbEdit != nullptr) {
                    value = std::to_string(pfd->enum_type()->FindValueByName(pCbEdit->currentText().toStdString().c_str())->number());
                    textValue = pCbEdit->currentText().toStdString();
                }
            } else if (pfd->type() == google::protobuf::FieldDescriptor::TYPE_BOOL) {
                std::string cbName = "cb_" + std::to_string(messageIdx);
                auto* pCbEdit = pBtn->parentWidget()->findChild<QComboBox*>(cbName.c_str());
                if (pCbEdit != nullptr) {
                    value = std::to_string(pCbEdit->currentIndex());
                    textValue = pCbEdit->currentText().toStdString();
                }
            } else {
                std::string lineEditName = "lineEdit_" + std::to_string(messageIdx);
                auto* pLineEdit = pBtn->parentWidget()->findChild<CLineEdit*>(lineEditName.c_str());
                if (pLineEdit != nullptr) {
                    value = pLineEdit->text().toStdString();
                    textValue = value;
                }
            }

            std::string listName = "list_" + std::to_string(messageIdx);
            pListWidget = pBtn->parentWidget()->findChild<QListWidget*>(listName.c_str());
        }
    }

    // 不能为空
    if (value.empty()) {
        return;
    }

    google::protobuf::FieldDescriptor::Type filedType = pfd->type();
    if (filedType != google::protobuf::FieldDescriptor::TYPE_STRING
        && filedType != google::protobuf::FieldDescriptor::TYPE_BYTES) {
        // checkif is numberic
        for (int i = 0; i < value.length(); ++i) {
            if (!isdigit(value[i])) {
                QMessageBox msgBox;
                msgBox.setText("You need enter digits.");
                msgBox.exec();
                return;
            }
        }
    }

    switch (filedType) {
    case google::protobuf::FieldDescriptor::TYPE_ENUM:
        m_pMessage->GetReflection()->AddEnumValue(m_pMessage, pfd, atoi(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_INT64:
        m_pMessage->GetReflection()->AddInt64(m_pMessage, pfd, atol(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_INT32:
        m_pMessage->GetReflection()->AddInt32(m_pMessage, pfd, atoi(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_UINT32:
        m_pMessage->GetReflection()->AddUInt32(m_pMessage, pfd, atoi(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_UINT64:
        m_pMessage->GetReflection()->AddUInt64(m_pMessage, pfd, atol(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
        m_pMessage->GetReflection()->AddDouble(m_pMessage, pfd, atol(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_FLOAT:
        m_pMessage->GetReflection()->AddFloat(m_pMessage, pfd, atof(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::TYPE_STRING:
    case google::protobuf::FieldDescriptor::TYPE_BYTES:
        m_pMessage->GetReflection()->AddString(m_pMessage, pfd, value);
        break;
    case google::protobuf::FieldDescriptor::TYPE_BOOL:
        m_pMessage->GetReflection()->AddBool(m_pMessage, pfd, bool(atoi(value.c_str()) != 0));
        break;
    default:
        break;
    }

    if (nullptr != pListWidget) {
        int curRow = pListWidget->currentRow();
        if (-1 == curRow) {
            curRow = 0;
        }

        int count = m_pMessage->GetReflection()->FieldSize(*m_pMessage, pfd);
        for (int i = count - 1; i > curRow; --i) {
            m_pMessage->GetReflection()->SwapElements(m_pMessage, pfd, i, i - 1);
        }

        pListWidget->insertItem(curRow, textValue.c_str());
    }
}

void CMsgEditorDialog::handleClearBtn() {
    auto* pBtn = static_cast<QPushButton*>(sender());
    int messageIdx = getBtnRegisterIndex(pBtn);

    const google::protobuf::FieldDescriptor* pFd = m_pMessage->GetDescriptor()->FindFieldByNumber(messageIdx);

    if (!pFd->is_repeated()) {
        return;        
    }

    if (pBtn->parentWidget() != nullptr) {
        if (pBtn->parentWidget()->layout() != nullptr) {
            std::string listName = "list_" + std::to_string(messageIdx);
            auto* pListWidget = pBtn->parentWidget()->findChild<QListWidget*>(listName.c_str());
            if (nullptr == pListWidget) {
                return;
            }

            QMessageBox msgBox;
            msgBox.setInformativeText("Do you want to clear?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Discard);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes) {
                m_pMessage->GetReflection()->ClearField(m_pMessage, pFd);
                pListWidget->clear();
            }
        }
    }
}

void CMsgEditorDialog::registerBtn(QPushButton* pBtn, int index) {
    m_mapBtn2MsgIdx[pBtn] = index;
}

int CMsgEditorDialog::getBtnRegisterIndex(QPushButton* pBtn) {
    if (nullptr != pBtn) {
        auto it = m_mapBtn2MsgIdx.find(pBtn);
        if (it != m_mapBtn2MsgIdx.end()) {
            return it->second;
        }
    }

    return -1;
}

void CMsgEditorDialog::setPlaceHodler(CLineEdit* pEdit, google::protobuf::FieldDescriptor::Type type) {
    if (nullptr != pEdit) {
        QString text = "";
        switch (type) {
        case google::protobuf::FieldDescriptor::TYPE_INT64:
            text = "INT64...";
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT32:
            text = "INT32...";
            break;
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
            text = "UINT32...";
            break;
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
            text = "UINT64...";
            break;
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            text = "DOUBLE...";
            break;
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            text = "FLOAT...";
            break;
        case google::protobuf::FieldDescriptor::TYPE_STRING:
            text = "STRING...";
            break;
        case google::protobuf::FieldDescriptor::TYPE_BYTES:
            text = "BYTES...";
            break;
        default:
            break;
        }
        pEdit->setPlaceholderText(text);
    }
}
