#include "MsgIgnoreDialog.h"
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>

#include "ProtoManager.h"

CMsgIgnoreDialog::CMsgIgnoreDialog(QWidget *parent)
    : QDialog(parent, Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint) {
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

void CMsgIgnoreDialog::init(std::unordered_set<std::string>& setIgnoredMsg) {
    std::list<CProtoManager::MsgInfo> listNames = ProtoManager::instance().getMsgInfos();
    {
        listMsgRight->clear();
        auto it = setIgnoredMsg.begin();
        for (; it != setIgnoredMsg.end(); ++it) {
            auto* pListItem = new QListWidgetItem();
            pListItem->setText((*it).c_str());
            listMsgRight->addItem(pListItem);
            listNames.remove_if([ ignoreFullName = *it](auto msgInfo) {
                return msgInfo.msgFullName == ignoreFullName;
            });
        }
    }

    {
        listMsgLeft->clear();
        auto it = listNames.begin();
        for (; it != listNames.end(); ++it) {
            auto* pListItem = new QListWidgetItem();
            pListItem->setText(it->msgFullName.c_str());
            listMsgLeft->addItem(pListItem);
        }
    }
}

std::list<std::string> CMsgIgnoreDialog::getIgnoredMsg() {
    std::list<std::string> listIgnoredMsgName;
    QListWidgetItem* pItem = nullptr;
    for (int i = 0; i < listMsgRight->count(); ++i) {
        pItem = listMsgRight->item(i);
        listIgnoredMsgName.emplace_back(pItem->text().toStdString());
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
        const CProtoManager::MsgInfo* pMsgInfo = ProtoManager::instance().getMsgInfoByMsgType(msgTypeNum);
        if (nullptr == pMsgInfo) {
            return;
        }

        std::string strMsgName = pMsgInfo->msgFullName;
        QList<QListWidgetItem*> listFound = listWidget.findItems(strMsgName.c_str(), Qt::MatchCaseSensitive);
        for (int i = 0; i < listFound.count(); ++i) {
            listFound[i]->setHidden(false);
        }
    } else {
        QString rexPattern;
        auto listText = filterText.split(QRegExp("[ |]"), QString::SkipEmptyParts);
        for (auto& word : listText) {
          word.prepend(".*");
          word.append(".*");
          rexPattern.append(word);
          rexPattern.append("|");
        }
        rexPattern.chop(1);
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
            pItem = listMsgLeft->takeItem(i);
            listMsgRight->addItem(pItem);
            i--;
        }
    }

    filterMessage(editFilterRight->text(), *listMsgRight);
}

void CMsgIgnoreDialog::handleAll2LeftBtnClicked() {
    for (int i = 0; i < listMsgRight->count(); ++i) {
        QListWidgetItem* pItem = listMsgRight->item(i);
        if (!pItem->isHidden()) {
            pItem = listMsgRight->takeItem(i);
            listMsgLeft->addItem(pItem);
            i--;
        }
    }
    filterMessage(editFilterLeft->text(), *listMsgLeft);
}

void CMsgIgnoreDialog::handleOne2RightBtnClicked() {
    QList<QListWidgetItem*> listSelected = listMsgLeft->selectedItems();
    for (int i = 0; i < listSelected.count(); ++i) {
        int nRow = listMsgLeft->row(listSelected[i]); 
        auto* pItem = listMsgLeft->takeItem(nRow);
        listMsgRight->addItem(pItem);
    }
    filterMessage(editFilterRight->text(), *listMsgRight);
}

void CMsgIgnoreDialog::handleOne2LeftBtnClicked() {
    QList<QListWidgetItem*> listSelected = listMsgRight->selectedItems();
    for (int i = 0; i < listSelected.count(); ++i) {
        int nRow = listMsgRight->row(listSelected[i]); 
        auto* pItem = listMsgRight->takeItem(nRow);
        listMsgLeft->addItem(pItem);
    }
    filterMessage(editFilterLeft->text(), *listMsgLeft);
}

