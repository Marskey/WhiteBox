#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_EmulatorClient.h"
#include "ModuleLogin.h"
#include "IClientNetEvent.h"
#include "MsgDetailDialog.h"
#include "ProtoManager.h"

#include <QSettings>
#include <QKeyEvent>
#include <QToolTip>
#include <QMessageBox>

#include "google/protobuf/message.h"
#include <google/protobuf/util/json_util.h>

static google::protobuf::util::JsonPrintOptions g_option;

class CEmulatorClient;
//must be in a header, otherwise moc gets confused with missing vtable
class DeleteHighlightedItemFilter : public QObject
{
     Q_OBJECT
protected:
    bool eventFilter(QObject *obj, QEvent *event) {
        if (event->type() == QEvent::KeyPress) {
            auto keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->modifiers() == Qt::ShiftModifier && keyEvent->key() == Qt::Key::Key_Delete) {
                auto itemView = qobject_cast<QAbstractItemView*>(obj);
                if (itemView->model() != nullptr) {
                    // Delete the current item from the popup list
                    itemView->model()->removeRow(itemView->currentIndex().row());
                    return true;
                }
            }
        }
        // Perform the usual event processing
        return QObject::eventFilter(obj, event);
    }
};

class ShowItemDetailKeyFilter : public QObject
{
     Q_OBJECT
protected:
    bool eventFilter(QObject *obj, QEvent *event) {
        if (event->type() == QEvent::KeyRelease) {
            auto keyEvent = static_cast<QKeyEvent*>(event);
            if (!keyEvent->isAutoRepeat()) {
                if (keyEvent->key() == Qt::Key::Key_Space) {
                    auto itemView = qobject_cast<QAbstractItemView*>(obj);
                    if (itemView->model() != nullptr) {
                        QModelIndex modelIdx = itemView->currentIndex();
                        QString msgFullName = modelIdx.data(Qt::WhatsThisRole).toString();
                        std::string detail = modelIdx.data(Qt::ToolTipRole).toString().toStdString();

                        if (detail.empty()) {
                            const google::protobuf::Descriptor* pDescriptor = ProtoManager::singleton().findMessageByName(msgFullName.toStdString());

                            if (pDescriptor == nullptr) {
                                return false;
                            }

                            google::protobuf::Message* pMessage = ProtoManager::singleton().createMessage(pDescriptor);
                            google::protobuf::util::MessageToJsonString(*pMessage, &detail, g_option);
                            delete pMessage;
                        }

                        QString msgName = modelIdx.data(Qt::DisplayRole).toString();
                        CMsgDetailDialog* pDlg = new CMsgDetailDialog(qApp->activeWindow(), msgName, detail.c_str());
                        pDlg->exec();
                        delete pDlg;

                        return true;
                    }
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

class CEmulatorClient : public QMainWindow, public IClientNetEvent
{
    struct ECVersion
    {
        int main;
        int sub;
        int build;
    };

    enum EConnectState
    {
        kConnected,
        kDisconnect,
        kConnecting,
    };

    typedef std::unordered_map<std::string, ::google::protobuf::Message*> MapMessages;
    Q_OBJECT

public:
    CEmulatorClient(QWidget *parent = Q_NULLPTR);

    static std::string highlightJsonData(const QString& jsonData);
public:
    virtual void doResolveMsg(const IMsg &msg) override;

    // Socket第一次连接上来触发
    virtual bool doOnConnect(const IMsg &msg, const std::string &strRemoteIp) override {
        return false;
    };

    // Socket断开连接触发
    virtual void doOnClose(SocketId unSocketID) override;

    // 主动连接成功后触发
    virtual void doDoConnect(SocketId unSocketID) override;

    // 提供网站连接 不需要解析消息头
    virtual bool doWebConnect(const IMsg &msg) override {
        return false;
    };

    virtual void onConnectFail(const std::string &strRemoteIp
                               , uint32_t unPort
                               , int32_t nConnnectType) override;
    // 连接服务器成功
    virtual void onConnectSuccess(const std::string &strRemoteIp
                                  , uint32_t unPort
                                  , int32_t nConnnectType
                                  , SocketId unSocketID) override;

private:
    void initQtObjectConnect();
    void connectStateChange(EConnectState state);

    bool ignoreMsgType(uint16_t uMsgType);
    google::protobuf::Message* getMessageByName(const char* name);
    google::protobuf::Message* getOrCreateMessageByName(const char* name);

    void log(std::string msg, QColor color = QColor(Qt::GlobalColor(0)), std::string detail = "");
    void ErrLog(std::string msg);
    void WarnLog(std::string msg);

public slots:
    void save();
    void load();
    void clear();
    void update();
    void reSelectProtoPath();

    void handleListMessageItemDoubleClicked(QListWidgetItem* pItem);
    void handleListMessageCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void handleListMessageItemClicked(QListWidgetItem* pItem);

    void handleListLogItemCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void handleListLogItemCliecked(QListWidgetItem* pItem);
    void handleFilterTextChanged(const QString& text);
    void handleSendBtnClicked();
    void handleConnectBtnClicked();

    void handleSearchDetailTextChanged();
    void handleSearchDetailBtnLastResult();
    void handleSearchDetailBtnNextResult();

    void handleSearchLogTextChanged();
    void handleSearchLogBtnLastResult();
    void handleSearchLogBtnNextResult();

    void handleBtnIgnoreMsgClicked();
    void handledStateChanged(int state);

protected:
    // 程序关闭时候的事件
    void closeEvent(QCloseEvent* event) override;

    // 按键事件
    void keyPressEvent(QKeyEvent* event) override;

private:
    Ui::EmulatorClientClass ui;
    MapMessages m_mapMessages;

    CModuleLogin m_loginModule;
    QSettings* m_configIni;
    std::set<uint16_t> m_setIgnoredReceiveMsgType;
    QSize m_subWindowSize;

    DeleteHighlightedItemFilter* m_cbKeyPressFilter = new DeleteHighlightedItemFilter();
    ShowItemDetailKeyFilter* m_listMessageKeyFilter = new ShowItemDetailKeyFilter();
    std::vector<int> m_vecSearchPos;
    int m_searchResultIdx = -1; // 当前高亮的第几个搜索结果(详情界面)

    QList<QListWidgetItem*> m_listFoundItems;
    int m_searchLogResultIdx = -1; // 当前高亮的第几个搜索结果(日志界面)
};

