#pragma once

#include "INetEvent.h"
#include "LogHelper.h"
#include "ConfigHelper.h"
#include "MsgDetailDialog.h"
#include "ProtoManager.h"
#include "Client.h"

#include "ui_MainWindow.h"
#include <QtWidgets/QMainWindow>
#include <QKeyEvent>

#include "google/protobuf/message.h"
#include <google/protobuf/util/json_util.h>

class ShowItemDetailKeyFilter;
class DeleteHighlightedItemFilter;
class CECPrinter;

class CMainWindow : public QMainWindow, public ec_net::INetEvent, public lua_api::IMainApp
{
    enum EConnectState
    {
        kConnected,
        kDisconnect,
        kConnecting,
    };

    typedef std::unordered_map<std::string, ::google::protobuf::Message*> MapMessages;
    Q_OBJECT

public:
    CMainWindow(QWidget* parent = Q_NULLPTR);
    ~CMainWindow();

    bool init();

    static std::string highlightJsonData(const QString& jsonData);

public:
    // ec_net::INetEvent begin
    void onParseMessage(SocketId socketId, const char* msgFullName, const char* pData, size_t size) override;
    void onConnectSucceed(const char* remoteIp
                          , Port port
                          , SocketId socketId) override;
    void onDisconnect(SocketId socketId) override;
    void onError(SocketId socketId, ec_net::ENetError error) override;
    // ec_net::INetEvent end

public slots:
    void saveCache();
    void loadCache();
    void clearCache();

    void update();
    void updateLog();

    void openSettingDlg();

    void handleListMessageItemDoubleClicked(QListWidgetItem* pItem);
    void handleListMessageCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void handleListMessageItemClicked(QListWidgetItem* pItem);

    void handleListLogItemCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void handleListLogItemClicked(QListWidgetItem* pItem);
    void handleListLogCustomContextMenuRequested(const QPoint& pos);
    void handleListLogActionAddToIgnoreList();

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

    void handleBtnClearLogClicked();

    void handleLogInfoAdded(const QModelIndex& parent, int start, int end);

protected:
    // 程序关闭时候的事件
    void closeEvent(QCloseEvent* event) override;
    void timerEvent(QTimerEvent *event) override;

    // 按键事件
    void keyPressEvent(QKeyEvent* event) override;

    void addNewItemIntoCombox(QComboBox& combox);

private:
    void clearProto();
    bool loadProto();
    bool loadLua();

    void doReload();

    ///注册类对象给lua
    void luaRegisterCppClass();

    void addDetailLogInfo(const char* msgFullName, const char* msg, const char* detail, QColor color = QColor(Qt::GlobalColor(0)));

    void connectStateChange(EConnectState state);

    bool ignoreMsgType(std::string msgFullName);
    google::protobuf::Message* getMessageByName(const char* name);
    google::protobuf::Message* getOrCreateMessageByName(const char* name);

    CClient* getClientBySocketId(SocketId id);

    // lua_api::IMainApp begin
    int addTimer(int interval) override;
    void deleteTimer(int timerId) override;
    lua_api::IClient* createClient(const char* name) override;
    lua_api::IClient* getClient(const char* name) override;
    /**
     * 给LUA脚本调用
     */
    void log(const char* message) override;
    // lua_api::IMainApp end
private:
    Ui::MainWindowClass ui;
    MapMessages m_mapMessages;
    std::unordered_set<std::string> m_setIgnoredReceiveMsgType;

    EConnectState m_btnConnectState = kDisconnect;

    DeleteHighlightedItemFilter* m_cbKeyPressFilter = nullptr;
    ShowItemDetailKeyFilter* m_listMessageKeyFilter = nullptr;
    std::vector<int> m_vecSearchPos;
    int m_searchResultIdx = -1; // 当前高亮的第几个搜索结果(详情界面)

    QList<QListWidgetItem*> m_listFoundItems;
    int m_searchLogResultIdx = -1; // 当前高亮的第几个搜索结果(日志界面)

    CECPrinter* m_pPrinter = nullptr;

    std::unordered_map<std::string, CClient*> m_mapClients;
    std::list<CClient*> m_listClientsToDel;

    QTimer* m_logUpdateTimer = nullptr;
};

class CECPrinter : public CLogPrinter
{
public:
    explicit CECPrinter(QListWidget* widget) : m_logWidget(widget) {
    }

    void onPrintError(const std::string& message) override {
        log(message, Qt::red);
    }

    void onPrintInfo(const std::string& message) override {
        log(message);
    }

    void onPrintWarning(const std::string& message) override {
        log(message, Qt::darkYellow);
    }

    void log(std::string msg, QColor color = QColor(Qt::GlobalColor(0))) {
        time_t t = time(0);
        char tmp[64];
        strftime(tmp, sizeof(tmp), "[%X] ", localtime(&t));

        msg = tmp + msg;
        auto* pListWidgetItem = new QListWidgetItem(msg.c_str(), m_logWidget);

        if (QColor(Qt::GlobalColor(0)) != color) {
            pListWidgetItem->setForeground(color);
        }

        m_logWidget->addItem(pListWidgetItem);
    }

    QListWidget* m_logWidget = nullptr;
};

class DeleteHighlightedItemFilter : public QObject
{
    Q_OBJECT
protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
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
    bool eventFilter(QObject* obj, QEvent* event) override {
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
                            google::protobuf::Message* pMessage = ProtoManager::instance().createMessage(msgFullName.toStdString().c_str());
                            if (pMessage == nullptr) {
                                return false;
                            }
                            google::protobuf::util::MessageToJsonString(*pMessage, &detail, ConfigHelper::instance().getJsonPrintOption());
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

