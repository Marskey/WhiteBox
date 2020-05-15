#include "MsgIgnoreDialog.h"
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <sstream>

#include "google/protobuf/repeated_field.h"
#include "ProtoManager.h"

CMsgIgnoreDialog::CMsgIgnoreDialog(QWidget *parent)
    : QDialog(parent) {
    setupUi(this);

    QObject::connect(editFilterLeft, SIGNAL(textChanged(const QString&)), this, SLOT(handleLeftFilterTextChanged(const QString&)));
    QObject::connect(editFilterRight, SIGNAL(textChanged(const QString&)), this, SLOT(handleRightFilterTextChanged(const QString&)));

    QObject::connect(btnAll2Right, SIGNAL(clicked()), this, SLOT(handleAll2RightBtnClicked()));
    QObject::connect(btnAll2Left, SIGNAL(clicked()), this, SLOT(handleAll2LeftBtnClicked()));
    QObject::connect(btnOne2Right, SIGNAL(clicked()), this, SLOT(handleOne2RightBtnClicked()));
    QObject::connect(btnOne2Left, SIGNAL(clicked()), this, SLOT(handleOne2LeftBtnClicked()));
}

CMsgIgnoreDialog::~CMsgIgnoreDialog() {
}

void CMsgIgnoreDialog::init(std::set<uint16_t>& setIgnoredMsg) {
    std::list<CProtoManager::MsgInfo> listNames = ProtoManager::singleton().getMsgInfos();
    {
        listMsgRight->clear();
        auto it = setIgnoredMsg.begin();
        for (; it != setIgnoredMsg.end(); ++it) {
            CProtoManager::MsgInfo msgInfo = ProtoManager::singleton().getMsgInfoByMsgType(*it);
            QListWidgetItem* pListItem = new QListWidgetItem();
            pListItem->setText(msgInfo.msgName.c_str());
            pListItem->setWhatsThis(msgInfo.msgFullName.c_str());
            listMsgRight->addItem(pListItem);
            listNames.remove(msgInfo);
        }
    }

    {
        listMsgLeft->clear();
        auto it = listNames.begin();
        for (; it != listNames.end(); ++it) {
            QListWidgetItem* pListItem = new QListWidgetItem();
            pListItem->setText(it->msgName.c_str());
            pListItem->setWhatsThis(it->msgFullName.c_str());
            listMsgLeft->addItem(pListItem);
        }
    }
}

std::list<std::string> CMsgIgnoreDialog::getIgnoredMsg() {
    std::list<std::string> listIgnoredMsgName;
    QListWidgetItem* pItem = nullptr;
    for (int i = 0; i < listMsgRight->count(); ++i) {
        pItem = listMsgRight->item(i);
        listIgnoredMsgName.emplace_back(pItem->whatsThis().toStdString());
    }
    return std::move(listIgnoredMsgName);
}

void CMsgIgnoreDialog::filterMessage(const QString& filterText, QListWidget& listWidget) {
    // 搜索栏搜索逻辑
    for (int i = 0; i < listWidget.count(); ++i) {
        listWidget.setItemHidden(listWidget.item(i), !filterText.isEmpty());
    }

    bool bIsNumberic = false;
    int msgTypeNum = filterText.toInt(&bIsNumberic); 
    if (bIsNumberic) {
        std::string strMsgName = ProtoManager::singleton().getMsgInfoByMsgType(msgTypeNum).msgName;
        QList<QListWidgetItem*> listFound = listWidget.findItems(strMsgName.c_str(), Qt::MatchCaseSensitive);
        for (int i = 0; i < listFound.count(); ++i) {
            listFound[i]->setHidden(false);
        }
    } else {
        QString rexPattern = filterText;
        rexPattern = rexPattern.replace(" ", ".*");
        rexPattern.prepend(".*");
        rexPattern.append(".*");
        QList<QListWidgetItem*> listFound = listWidget.findItems(rexPattern, Qt::MatchRegExp);
        for (int i = 0; i < listFound.count(); ++i) {
            listFound[i]->setHidden(false);
        }
    }
}

void CMsgIgnoreDialog::handleLeftFilterTextChanged(const QString& text) {
    filterMessage(text, *listMsgLeft);
}

void CMsgIgnoreDialog::handleRightFilterTextChanged(const QString& text) {
    filterMessage(text, *listMsgRight);
}

void CMsgIgnoreDialog::handleAll2RightBtnClicked() {
    for (int i = 0; i < listMsgLeft->count(); ++i) {
        QListWidgetItem* pItem = listMsgLeft->item(i);
        if (!pItem->isHidden()) {
            listMsgRight->addItem(new QListWidgetItem(*pItem));
            delete listMsgLeft->takeItem(i);
            i--;
        }
    }

    filterMessage(editFilterRight->text(), *listMsgRight);
}

void CMsgIgnoreDialog::handleAll2LeftBtnClicked() {
    for (int i = 0; i < listMsgRight->count(); ++i) {
        QListWidgetItem* pItem = listMsgRight->item(i);
        if (!pItem->isHidden()) {
            listMsgLeft->addItem(new QListWidgetItem(*pItem));
            delete listMsgRight->takeItem(i);
            i--;
        }
    }
    filterMessage(editFilterLeft->text(), *listMsgLeft);
}

void CMsgIgnoreDialog::handleOne2RightBtnClicked() {
    QList<QListWidgetItem*> listSelected = listMsgLeft->selectedItems();
    for (int i = 0; i < listSelected.count(); ++i) {
        QListWidgetItem* pItem = new QListWidgetItem(*listSelected[i]);
        listMsgRight->addItem(pItem);
        delete listSelected[i];
    }
    filterMessage(editFilterRight->text(), *listMsgRight);
}

void CMsgIgnoreDialog::handleOne2LeftBtnClicked() {
    QList<QListWidgetItem*> listSelected = listMsgRight->selectedItems();
    for (int i = 0; i < listSelected.count(); ++i) {
        QListWidgetItem* pItem = new QListWidgetItem(*listSelected[i]);
        listMsgLeft->addItem(pItem);
        delete listSelected[i];
    }
    filterMessage(editFilterLeft->text(), *listMsgLeft);
}

