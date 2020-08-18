#include "MsgEditorDialog.h"
#include "LineEdit.h"
#include "JsonHighlighter.h"

#include <QFormLayout>
#include <QListWidget>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QTextEdit>


#include "ConfigHelper.h"
#include "google/protobuf/repeated_field.h"
#include "ProtoManager.h"

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

void CMsgEditorDialog::initDialogByMessage(const ::google::protobuf::Message* pMessage) {
    m_pMessage = pMessage->New();
    m_pMessage->CopyFrom(*pMessage);

    m_isAnyType = "google.protobuf.Any" == pMessage->GetTypeName();

    // 设置标题
    labelTitle->setText(m_pMessage->GetTypeName().c_str());

    auto* scrollAreaContent = new QWidget(this);
    auto* pFormLayout = new QFormLayout(scrollAreaContent);

    if (!m_isAnyType) {
        int cnt = pMessage->GetDescriptor()->field_count();
        for (int i = 0; i < cnt; ++i) {
            const google::protobuf::FieldDescriptor* pFd = pMessage->GetDescriptor()->field(i);
            createWidget(*pFormLayout, pFd->name(), pFd);
        }
    } else {
        tabWidget->removeTab(static_cast<int>(ETabIdx::eGUI));
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

    auto* pFieldLabel = new QLabel(fmt::format("<h6>{}</h6> <span style=\"color: #569ad6\">{}</span>"
                                               , strFiledName
                                               , pDescriptor->cpp_type_name()).c_str()
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
            pListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
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
                pComboBox->setItemDelegate(new QStyledItemDelegate());
                pComboBox->setObjectName(("cb_" + std::to_string(pDescriptor->number())).c_str());

                for (int i = 0; i < pEd->value_count(); ++i) {
                    const google::protobuf::EnumValueDescriptor* value = pEd->value(i);
                    std::string enumName = value->name();

                    pComboBox->addItem(enumName.c_str());
                }
                pHBoxLayout->addWidget(pComboBox);
                
            } else if (pDescriptor->type() == google::protobuf::FieldDescriptor::TYPE_BOOL) {
                auto* pComboBox = new QComboBox(this);
                pComboBox->setItemDelegate(new QStyledItemDelegate());
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
            pListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
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
        std::string msgValue = ProtoManager::instance().getMsgValue(*m_pMessage, pDescriptor);

        switch (filedType) {
            case google::protobuf::FieldDescriptor::TYPE_ENUM:
            {
                const google::protobuf::EnumDescriptor* pEd = pDescriptor->enum_type();

                // enum select
                auto* pComboBox = new QComboBox(this);
                pComboBox->setItemDelegate(new QStyledItemDelegate());
                pComboBox->setObjectName(std::to_string(pDescriptor->number()).c_str());
                QObject::connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(handleComboxIndexChanged(int)));

                for (int i = 0; i < pEd->value_count(); ++i) {
                    const google::protobuf::EnumValueDescriptor* value = pEd->value(i);
                    std::string enumName = value->name();

                    pComboBox->addItem(enumName.c_str());
                }

                pComboBox->setCurrentText(msgValue.c_str());

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
                // normal input
                auto* pLineEdit = new CLineEdit(this);
                pLineEdit->setObjectName(std::to_string(pDescriptor->number()).c_str());
                if (!msgValue.empty()) {
                    pLineEdit->setText(msgValue.c_str());
                }
                QObject::connect(pLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(handleLineEdit(const QString&)));
                layout.addRow(pFieldLabel, pLineEdit);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_STRING:
            case google::protobuf::FieldDescriptor::TYPE_BYTES:
            {
                // normal input
                auto* pLineEdit = new CLineEdit(this);
                pLineEdit->setObjectName(std::to_string(pDescriptor->number()).c_str());
                if (!msgValue.empty()) {
                    pLineEdit->setText(msgValue.c_str());
                }
                QObject::connect(pLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(handleLineEdit(const QString&)));
                layout.addRow(pFieldLabel, pLineEdit);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_BOOL:
            {
                auto* pComboBox = new QComboBox(this);
                pComboBox->setItemDelegate(new QStyledItemDelegate());
                pComboBox->addItem("false", 0);
                pComboBox->addItem("true", 1);
                pComboBox->setObjectName(std::to_string(pDescriptor->number()).c_str());
                pComboBox->setCurrentText(msgValue.c_str());
                QObject::connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(handleComboxIndexChanged(int)));
                layout.addRow(pFieldLabel, pComboBox);
                break;
            }
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
                connect(pBtn2, SIGNAL(clicked()), this, SLOT(handleClearBtn()));

                auto* pTextEdit = new QTextEdit(this);
                pTextEdit->setReadOnly(true);
                pTextEdit->setObjectName(std::to_string(pDescriptor->number()).c_str());
                auto* pVBoxLayout = new QVBoxLayout(this);
                pVBoxLayout->addLayout(pHBoxLayout);
                pVBoxLayout->addWidget(pTextEdit);
                pTextEdit->setText(msgValue.c_str());

                layout.addRow(pFieldLabel, pVBoxLayout);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_GROUP:
                break;
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
    auto* pDlg = new CMsgEditorDialog(this);
    pDlg->initDialogByMessage(pEditMessage);
    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        pEditMessage->CopyFrom(pDlg->getMessage());
        if (pBtn->parentWidget() != nullptr) {
            if (pBtn->parentWidget()->layout() != nullptr) {
                auto pTextEdit  = pBtn->parentWidget()->findChild<QTextEdit*>(std::to_string(pFd->number()).c_str());

                if (0 != pEditMessage->ByteSizeLong()) {
                    pTextEdit->setText(pEditMessage->Utf8DebugString().c_str());
                }
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

                if (0 != pNewMessage->ByteSizeLong()) {
                    pListWidget->addItem(pNewMessage->ShortDebugString().c_str());
                }
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

                if (0 != pNewMessage->ByteSizeLong()) {
                    pListWidget->insertItem(curRow, pNewMessage->ShortDebugString().c_str());
                }
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

    QMessageBox msgBox;
    msgBox.setInformativeText("Do you want to clear?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Discard);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if (ret != QMessageBox::Yes) {
        return;
    }

    if (pFd->is_repeated()) {
        if (pBtn->parentWidget() != nullptr) {
            if (pBtn->parentWidget()->layout() != nullptr) {
                std::string listName = "list_" + std::to_string(messageIdx);
                auto* pListWidget = pBtn->parentWidget()->findChild<QListWidget*>(listName.c_str());
                if (nullptr == pListWidget) {
                    return;
                }

                m_pMessage->GetReflection()->ClearField(m_pMessage, pFd);
                pListWidget->clear();
            }
        }
    } else {
        if (pBtn->parentWidget() != nullptr) {
            if (pBtn->parentWidget()->layout() != nullptr) {
                auto* pTextEditWidget = pBtn->parentWidget()->findChild<QTextEdit*>(std::to_string(messageIdx).c_str());
                if (nullptr == pTextEditWidget) {
                    return;
                }

                m_pMessage->GetReflection()->ClearField(m_pMessage, pFd);
                pTextEditWidget->clear();
            }
        }
    }
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

void CMsgEditorDialog::updateGUIData() {
    int cnt = m_pMessage->GetDescriptor()->field_count();
    for (int i = 0; i < cnt; ++i) {
        const google::protobuf::FieldDescriptor* pDescriptor = m_pMessage->GetDescriptor()->field(i);
        if (nullptr == pDescriptor) {
            continue;
        }

        if (pDescriptor->is_repeated()) {
            std::string listName = "list_" + std::to_string(pDescriptor->number());
            auto* pListWidget = findChild<QListWidget*>(listName.c_str());
            if (nullptr == pListWidget) {
                continue;
            }
            pListWidget->clear();
            int size = m_pMessage->GetReflection()->FieldSize(*m_pMessage, pDescriptor);
            for (int i = 0; i < size; ++i) {
                pListWidget->addItem(ProtoManager::instance().getMsgValue(*m_pMessage, pDescriptor, i).c_str());
            }
        } else {
            std::string value = ProtoManager::instance().getMsgValue(*m_pMessage, pDescriptor);
            google::protobuf::FieldDescriptor::Type filedType = pDescriptor->type();
            switch (filedType) {
            case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            case google::protobuf::FieldDescriptor::TYPE_INT64:
            case google::protobuf::FieldDescriptor::TYPE_UINT64:
            case google::protobuf::FieldDescriptor::TYPE_INT32:
            case google::protobuf::FieldDescriptor::TYPE_FIXED64:
            case google::protobuf::FieldDescriptor::TYPE_FIXED32:
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
            case google::protobuf::FieldDescriptor::TYPE_SINT32:
            case google::protobuf::FieldDescriptor::TYPE_SINT64:
            case google::protobuf::FieldDescriptor::TYPE_STRING:
            case google::protobuf::FieldDescriptor::TYPE_BYTES:
                {
                    auto* pLineEdit = findChild<QLineEdit*>(std::to_string(pDescriptor->number()).c_str());
                    if (nullptr == pLineEdit) {
                        continue;
                    }

                    pLineEdit->setText(value.c_str());
                }
                break;
            case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
                {
                    auto* pTextEdit = findChild<QTextEdit*>(std::to_string(pDescriptor->number()).c_str());
                    if (nullptr == pTextEdit) {
                        continue;
                    }

                    pTextEdit->setText(value.c_str());
                }
                break;
            case google::protobuf::FieldDescriptor::TYPE_BOOL:
            case google::protobuf::FieldDescriptor::TYPE_ENUM:
                {
                    auto* pCombox = findChild<QComboBox*>(std::to_string(pDescriptor->number()).c_str());
                    if (nullptr == pCombox) {
                        continue;
                    }

                    pCombox->setCurrentText(value.c_str());
                }
                break;
            case google::protobuf::FieldDescriptor::TYPE_GROUP:
                break;
            default: ;
            }
        }
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

    accept();
}
