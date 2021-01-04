#include "MainWindow.h"
#include "ProtoManager.h"
#include "NetManager.h"
#include "Client.h"
#include "ip4validator.h"
#include "MsgEditorDialog.h"
#include "MsgIgnoreDialog.h"
#include "SettingDialog.h"
#include "ConfigHelper.h"
#include "LuaInterface.h"
#include "LuaScriptSystem.h"
#include "JsonHighlighter.h"
#include "LineEdit.h"

extern "C" {
#include "protobuf/protobuflib.h"
#include "cjson/lua_cjson.h"
}

#include <QTime>
#include <QTimer>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDirIterator>
#include <QStyledItemDelegate>
#include <QToolTip>

const char WINDOWS_TITLE[25] = "WhiteBox";

struct ECVersion
{
    int main;
    int sub;
    int build;
} const g_version = { 3 , 1 , 5 };

CMainWindow::CMainWindow(QWidget* parent)
    : QMainWindow(parent) {
    ui.setupUi(this);

    setWindowTitle(WINDOWS_TITLE);

    // 用户名
    {
        auto* pLineEdit = new CLineEdit(ui.cbAccount);
        pLineEdit->setPlaceholderText("Account");
        ui.cbAccount->setLineEdit(pLineEdit);
    }
    ui.cbAccount->view()->installEventFilter(new DeleteHighlightedItemFilter(ui.cbAccount));
    ui.cbAccount->view()->setItemDelegate(new PopupItemDelegate(ui.cbAccount));
    ConfigHelper::instance().restoreWidgetComboxState("Account", *ui.cbAccount);

    // ip端口
    {
        auto* pLineEdit = new CLineEdit(ui.cbIp);
        auto* pAddrValidator = new IP4Validator(ui.cbIp);
        pLineEdit->setValidator(pAddrValidator);
        pLineEdit->setPlaceholderText("IP:Port");
        ui.cbIp->setLineEdit(pLineEdit);
    }
    ui.cbIp->view()->installEventFilter(new DeleteHighlightedItemFilter(ui.cbIp));
    ui.cbIp->view()->setItemDelegate(new PopupItemDelegate(ui.cbIp));
    ConfigHelper::instance().restoreWidgetComboxState("Ip_Port", *ui.cbIp);

    // 可选项参数
    {
        auto* pLineEdit = new CLineEdit(ui.cbIp);
        pLineEdit->setPlaceholderText("Optional");
        ui.cbOptionalParam->setLineEdit(pLineEdit);
    }
    ui.cbOptionalParam->view()->installEventFilter(new DeleteHighlightedItemFilter(ui.cbOptionalParam));
    ui.cbOptionalParam->view()->setItemDelegate(new PopupItemDelegate(ui.cbOptionalParam));
    ConfigHelper::instance().restoreWidgetComboxState("OptionalParam", *ui.cbOptionalParam);

    ui.cbClientName->view()->setItemDelegate(new PopupItemDelegate(ui.cbClientName));

    // 读取是否自动显示最新
    ConfigHelper::instance().restoreWidgetCheckboxState("AutoShowDetail", *ui.checkIsAutoDetail);

    // 恢复上次的布局大小
    restoreGeometry(ConfigHelper::instance().getMainWindowGeometry());
    restoreState(ConfigHelper::instance().getMainWindowState());
    ui.splitterH->restoreState(ConfigHelper::instance().getSplitterH());
    ui.splitterV->restoreState(ConfigHelper::instance().getSplitterV());

    // 给log窗口添加右键菜单
    ui.listLogs->setContextMenuPolicy(Qt::CustomContextMenu);

    ui.btnSend->setDisabled(true);

    m_pMaskWidget = new QWidget(this, Qt::FramelessWindowHint);
    m_pMaskWidget->setStyleSheet("QWidget{background-color:rgba(0,0,0,0.5);}");
    m_pMaskWidget->hide();
    QLabel* pLoadingLabel = new QLabel("<font color=\"white\">Loading...</font>", m_pMaskWidget);
    QFont font( "", 28, QFont::Bold);
    pLoadingLabel->setFont(font);

    // 设置json数据高亮
    m_highlighter = new CJsonHighlighter(ui.plainTextEdit->document());

    ui.listMessage->setMouseTracking(true);



    QObject::connect(ui.listMessage, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(handleListMessageItemDoubleClicked(QListWidgetItem*)));

    QObject::connect(ui.listRecentMessage, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(handleListMessageItemDoubleClicked(QListWidgetItem*)));

    QObject::connect(ui.listMessage, SIGNAL(itemEntered(QListWidgetItem*)), this, SLOT(handleMouseEntered(QListWidgetItem*)));
    QObject::connect(ui.listRecentMessage, SIGNAL(itemEntered(QListWidgetItem*)), this, SLOT(handleMouseEntered(QListWidgetItem*)));

    QObject::connect(ui.listLogs, SIGNAL(currentRowChanged(int)), this, SLOT(handleListLogItemCurrentRowChanged(int)));

    QObject::connect(ui.listLogs->model(), SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(handleLogInfoAdded(const QModelIndex&, int, int)));
    QObject::connect(ui.listLogs, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(handleListLogCustomContextMenuRequested(const QPoint&)));

    QObject::connect(ui.editSearch, SIGNAL(textChanged(const QString&)), this, SLOT(handleFilterTextChanged(const QString&)));

    QObject::connect(ui.btnSend, SIGNAL(clicked()), this, SLOT(handleSendBtnClicked()));
    QObject::connect(ui.btnConnect, SIGNAL(clicked()), this, SLOT(handleConnectBtnClicked()));
    QObject::connect(ui.editSearchDetail, SIGNAL(textChanged(const QString&)), this, SLOT(handleSearchDetailTextChanged()));
    QObject::connect(ui.btnLastResult, SIGNAL(clicked()), this, SLOT(handleSearchDetailBtnLastResult()));
    QObject::connect(ui.btnNextResult, SIGNAL(clicked()), this, SLOT(handleSearchDetailBtnNextResult()));

    QObject::connect(ui.editLogSearch, SIGNAL(textChanged(const QString&)), this, SLOT(handleSearchLogTextChanged()));
    QObject::connect(ui.btnLogLastResult, SIGNAL(clicked()), this, SLOT(handleSearchLogBtnLastResult()));
    QObject::connect(ui.btnLogNextResult, SIGNAL(clicked()), this, SLOT(handleSearchLogBtnNextResult()));

    QObject::connect(ui.btnIgnoreMsg, SIGNAL(clicked()), this, SLOT(handleBtnIgnoreMsgClicked()));
    QObject::connect(ui.btnClearLog, SIGNAL(clicked()), this, SLOT(handleBtnClearLogClicked()));

    QObject::connect(ui.actionSave, &QAction::triggered, this, &CMainWindow::saveCache);
    QObject::connect(ui.actionLoad, &QAction::triggered, this, &CMainWindow::loadCache);
    QObject::connect(ui.actionClear, &QAction::triggered, this, &CMainWindow::clearCache);
    QObject::connect(ui.actionOptions, &QAction::triggered, this, &CMainWindow::openSettingDlg);
    QObject::connect(ui.actionReload_script, &QAction::triggered, this, &CMainWindow::doReload);
}

CMainWindow::~CMainWindow() {
    delete m_pPrinter;
    
    NetManager::instance().stop();

    for (auto& [key, value] : m_mapMessages) {
        delete value;
    }
    m_mapMessages.clear();

    for (auto& [key, client] : m_mapClients) {
        delete client;
    }
    m_mapClients.clear();
}

bool CMainWindow::init() {
    if (ConfigHelper::instance().isFirstCreateFile()) {
        auto* pDlg = new CSettingDialog(this);
        pDlg->init(true);
        int ret = pDlg->exec();
        if (ret == QDialog::Rejected) {
            ConfigHelper::instance().deleteConfigFile();
            exit(0);
        }
        delete pDlg;
    }
    // 初始化日志服务
    m_pPrinter = new CECPrinter(ui.listLogs);
    LogHelper::instance().setPrinter(m_pPrinter);

    // 初始化通信服务
    NetManager::instance().init(this);
    LOG_INFO("Initiated network");

    // 启动主定时器
    auto* pMainTimer = new QTimer(this);
    connect(pMainTimer, &QTimer::timeout, this, QOverload<>::of(&CMainWindow::update));
    pMainTimer->start(40);

    m_pLogUpdateTimer = new QTimer(this);
    connect(m_pLogUpdateTimer, &QTimer::timeout, this, QOverload<>::of(&CMainWindow::updateLog));

    // 加载lua
    if (!loadLua()) {
        return false;
    }

    // 加载proto文件
    if (!loadProto()) {
        return false;
    }

    // 加载缓存 
    loadCache();

    LOG_INFO("Ready.             - Version {}.{}.{} By marskey.", g_version.main, g_version.sub, g_version.build);
    return true;
}

void CMainWindow::connectStateChange(EConnectState state) {
    switch (state) {
    case EConnectState::kConnected:
        ui.btnConnect->setText("Disconnect");
        ui.btnConnect->setDisabled(false);
        ui.btnSend->setDisabled(false);

        ui.cbIp->setDisabled(true);
        ui.cbAccount->setDisabled(true);
        ui.cbOptionalParam->setDisabled(true);
        break;
    case EConnectState::kDisconnect:
        ui.btnConnect->setText("Connect");
        ui.btnConnect->setDisabled(false);
        ui.btnSend->setDisabled(true);

        ui.cbIp->setDisabled(false);
        ui.cbAccount->setDisabled(false);
        ui.cbOptionalParam->setDisabled(false);
        break;
    case EConnectState::kConnecting:
        ui.btnConnect->setText("Connecting");
        ui.btnConnect->setDisabled(true);
        ui.btnSend->setDisabled(true);

        ui.cbIp->setDisabled(true);
        ui.cbAccount->setDisabled(true);
        ui.cbOptionalParam->setDisabled(true);
        break;
    default:
        break;
    }
    m_btnConnectState = state;
}

bool CMainWindow::ignoreMsgType(std::string msgFullName) {
    if (m_setIgnoredReceiveMsgType.find(msgFullName) != m_setIgnoredReceiveMsgType.end()) {
        return true;
    }

    return false;
}

google::protobuf::Message* CMainWindow::getMessageByName(const char* name) {
    auto itr = m_mapMessages.find(name);
    if (itr == m_mapMessages.end()) {
        return nullptr;
    }

    return itr->second;
}

google::protobuf::Message* CMainWindow::getOrCreateMessageByName(const char* name) {
    auto itr = m_mapMessages.find(name);
    if (itr == m_mapMessages.end()) {
        auto* pMessage = ProtoManager::instance().createMessage(name);
        if (nullptr == pMessage) {
            LOG_INFO("Cannot find code of message({0})", name);
            return nullptr;
        }

        m_mapMessages[name] = pMessage;
        itr = m_mapMessages.find(name);
    }

    return itr->second;
}

CClient* CMainWindow::getClientBySocketId(SocketId id) {
    for (auto& [key, client] : m_mapClients) {
        if (id == client->getSocketID()) {
            return client;
        }
    }
    return nullptr;
}

int CMainWindow::addTimer(int interval) {
    return startTimer(interval);
}

void CMainWindow::deleteTimer(int timerId) {
    if (timerId > 0) {
        killTimer(timerId);
    }
}

lua_api::IClient* CMainWindow::createClient(const char* name) {
    if (strlen(name) == 0) {
        LOG_ERR("Client named cannot be empty");
        return nullptr;
    }

    auto* pClient = static_cast<CClient*>(getClient(name));
    if (pClient == nullptr) {
        pClient = new CClient();
        pClient->setName(name);
        m_mapClients[name] = pClient;
    } else if (pClient->isConnected()) {
        LOG_ERR("Already have a connected client named \"{}\"", name);
        return nullptr;
    } 

    return pClient;
}

lua_api::IClient* CMainWindow::getClient(const char* name) {
    if (strlen(name) == 0) {
        LOG_ERR("Client named cannot be empty");
        return nullptr;
    }

    auto it = m_mapClients.find(name);
    if (it != m_mapClients.end()) {
        return it->second;
    }
    return nullptr;
}

void CMainWindow::log(const char* message) {
    m_pPrinter->log(message);
}

void CMainWindow::logErr(const char* message) {
    m_pPrinter->log(message, Qt::red);
}

void CMainWindow::saveCache() {
    const std::string& path = ConfigHelper::instance().getCachePath().toStdString();
    QFile jsonFile(path.c_str());
    jsonFile.open(QIODevice::WriteOnly | QIODevice::Text);
    if (!jsonFile.isOpen()) {
        QMessageBox::warning(this, "", fmt::format("Cannot save cache file: {0}", path).c_str());
        return;
    }

    QJsonArray msgCacheArray;
    for (const auto& pair : m_mapMessages) {
        QJsonObject json;
        json["name"] = pair.first.c_str();
        std::string msgData;
        pair.second->SerializeToString(&msgData);
        QByteArray base64 = msgData.c_str();
        json["data"] = base64.toBase64().toStdString().c_str();
        msgCacheArray.append(json);
    }

    QJsonArray ignoredMsgArray;
    for (const auto& name : m_setIgnoredReceiveMsgType) {
        ignoredMsgArray.append(name.c_str());
    }

    QJsonObject jsonObj;
    jsonObj["msgCache"] = msgCacheArray;
    jsonObj["ignoredMsg"] = ignoredMsgArray;

    QJsonDocument jsonDoc(jsonObj);
    jsonFile.write(jsonDoc.toJson());
    jsonFile.close();
}

void CMainWindow::loadCache() {
    std::string path = ConfigHelper::instance().getCachePath().toStdString();
    if (path.empty()) {
        path = "./cache.json";
        ConfigHelper::instance().saveCachePath(path.c_str());
        return;
    }

    LOG_INFO("Loading cache...");
    QFile jsonFile(path.c_str());
    jsonFile.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!jsonFile.isOpen()) {
        LOG_WARN("Warning: no cache file:{} were found.", path);
        return;
    }

    if (!jsonFile.isReadable()) {
        LOG_ERR("Error: cannot read file:{}.", path);
        return;
    }

    QJsonParseError error{};
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll(), &error);
    jsonFile.close();

    if (error.error != QJsonParseError::NoError) {
        LOG_ERR("Error: cache parse failed: {}", error.errorString().toStdString());
    }

    if (jsonDoc.isEmpty()) {
        return;
    }

    int msgCacheFailedCnt = 0;
    QJsonObject jsonObj = jsonDoc.object();
    QJsonArray msgCacheArray = jsonObj["msgCache"].toArray();
    for (const QJsonValueRef& cache : msgCacheArray) {
        QJsonObject obj = cache.toObject();
        std::string msgName = obj["name"].toString().toStdString();
        google::protobuf::Message* pMessage = getOrCreateMessageByName(msgName.c_str());
        if (nullptr == pMessage) {
            LOG_ERR("Error: Cache file {} is corrupted!", path);
            ++msgCacheFailedCnt;
            continue;
        }

        std::string msgData = QByteArray::fromBase64(obj["data"].toString().toStdString().c_str()).toStdString();
        if (pMessage->ParseFromString(msgData)) {
            std::string msgStr;
            for (int i = 0; i < ui.listMessage->count(); ++i) {
                QListWidgetItem* pItem = ui.listMessage->item(i);
                if (nullptr != pItem) {
                    std::string msgFullName = pItem->data(Qt::UserRole + kMessageFullName).toString().toStdString();
                    if (msgFullName == msgName) {
                        pItem->setIcon(QIcon(":/WhiteBox/icon1.ico"));
                        google::protobuf::util::MessageToJsonString(*pMessage, &msgStr, ConfigHelper::instance().getJsonPrintOption());
                        //pItem->setToolTip(msgStr.c_str());
                        pItem->setData(Qt::UserRole + kMessageData, msgStr.c_str());
                    }
                }
            }
        }
        else {
          delete pMessage;
          m_mapMessages.erase(msgName);
        }
    }
    ui.labelCacheCnt->setText(std::to_string(m_mapMessages.size()).c_str());

    int ignoreMsgFailedCnt = 0;
    QJsonArray ignoredMsgArray = jsonObj["ignoredMsg"].toArray();
    for (const QJsonValueRef& cache : ignoredMsgArray) {
        m_setIgnoredReceiveMsgType.insert(cache.toString().toStdString());
    }
    ui.lableIgnoreCnt->setText(std::to_string(m_setIgnoredReceiveMsgType.size()).c_str());

    LOG_INFO("Cache loaded messages succeed: {}, failed: {}, Ignore message succeed: {}, failed: {}."
             , m_mapMessages.size()
             , msgCacheFailedCnt
             , m_setIgnoredReceiveMsgType.size()
             , ignoreMsgFailedCnt
    );
}

void CMainWindow::clearCache() {
    size_t count = m_mapMessages.size();
    m_mapMessages.clear();
    for (int i = 0; i < ui.listMessage->count(); ++i) {
        QListWidgetItem* pListWidgetItem = ui.listMessage->item(i);
        if (nullptr != pListWidgetItem) {
            pListWidgetItem->setIcon(QIcon());
            //pListWidgetItem->setToolTip("");
            pListWidgetItem->setData(Qt::UserRole + kMessageData, "");
        }
    }
    ui.labelCacheCnt->setText("0");

    for (int i = 0; i < ui.listRecentMessage->count(); ++i) {
        QListWidgetItem* pListWidgetItem = ui.listRecentMessage->item(i);
        if (nullptr != pListWidgetItem) {
            //pListWidgetItem->setToolTip("");
            pListWidgetItem->setData(Qt::UserRole + kMessageData, "");
        }
    }
}

void CMainWindow::update() {
    NetManager::instance().run();

    for (auto* client : m_listClientsToDel) {
        m_mapClients.erase(client->getName());
        delete client;
    }
    m_listClientsToDel.clear();
}

void CMainWindow::updateLog() {
    if (ui.checkIsAutoDetail->isChecked()) {
        ui.listLogs->setCurrentRow(ui.listLogs->count() - 1);
    } else {
        m_pLogUpdateTimer->stop();
    }
}

void CMainWindow::openSettingDlg() {
    auto* pDlg = new CSettingDialog(this);
    pDlg->init(false);
    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        doReload();
    }
    delete pDlg;
}

void CMainWindow::onParseMessage(SocketId socketId, const char* msgFullName, const char* pData, size_t size) {
    if (msgFullName == nullptr) {
        LOG_ERR("Message name is nullptr, check lua script's function \"bindMessage\"", msgFullName);
        return;
    }

    if (nullptr == pData) {
        LOG_ERR("Message data is nullptr, check lua script's function \"bindMessage\"", msgFullName);
        return;
    }

    uint16_t nMessageId = ProtoManager::instance().getMsgTypeByFullName(msgFullName);
    google::protobuf::Message* pRecvMesage = ProtoManager::instance().createMessage(msgFullName);
    if (nullptr == pRecvMesage) {
        LOG_INFO("Cannot find code of message({0}:{1})", msgFullName, nMessageId);
        return;
    }

    if (pRecvMesage->ParseFromArray(pData, size)) {
        // 如果添加忽略则不打印到log
        if (!ignoreMsgType(msgFullName)) {
            std::string msgStr;
            google::protobuf::util::MessageToJsonString(*pRecvMesage, &msgStr, ConfigHelper::instance().getJsonPrintOption());
            addDetailLogInfo(msgFullName
                             , fmt::format("Received <font color='{0}'>{2}</font> (<font color='{1}'>{3}</font>) size: <font color='{1}'>{4}</font>"
                                           , "#739d39"
                                           , "#3498DB"
                                           , msgFullName
                                           , nMessageId
                                           , pRecvMesage->ByteSizeLong())
                             , msgStr.c_str()
            );
        }

        CClient* pClient = getClientBySocketId(socketId);
        if (nullptr == pClient) {
            LOG_WARN("Cannot find client whose socket id is {}.", socketId);
            return;
        }
        LuaScriptSystem::instance().Invoke("__APP_on_message_recv"
                                           , static_cast<lua_api::IClient*>(pClient)
                                           , msgFullName
                                           , (void*)(&pRecvMesage));
    } else {
        LOG_ERR("Protobuf message {}({}) parse failed!", msgFullName, nMessageId);
    }

    delete pRecvMesage;
}

void CMainWindow::onConnectSucceed(const char* remoteIp, Port port, SocketId socketId) {
    LOG_INFO("Connect succeed socket id: {0}, ip: {1}:{2} ", socketId, remoteIp, port);
    connectStateChange(EConnectState::kConnected);

    CClient* pClient = getClientBySocketId(socketId);
    if (nullptr != pClient) {
        ui.cbClientName->addItem(fmt::format("[socket:{}] {}", socketId, pClient->getName()).c_str()
                                 , pClient->getName());
        pClient->onConnectSucceed(remoteIp, port, socketId);
    }
}

void CMainWindow::onDisconnect(SocketId socketId) {
    LOG_ERR("Connection closed, socket id: {}", socketId);
    for (auto& [key, client] : m_mapClients) {
        if (client->getSocketID() == socketId) {
            client->onDisconnect(socketId);
            m_listClientsToDel.insert(client);
            int cbIdx = ui.cbClientName->findData(client->getName());
            if (-1 != cbIdx) {
                ui.cbClientName->removeItem(cbIdx);
            }
            break;
        }
    }

    if (ui.cbClientName->count() == 0) {
        connectStateChange(EConnectState::kDisconnect);
    }
}

void CMainWindow::onError(SocketId socketId, ec_net::ENetError error) {
    switch (error) {
    case ec_net::eNET_CONNECT_FAIL:
        {
            std::string remoteIp = NetManager::instance().getRemoteIP(socketId);
            Port port = NetManager::instance().getRemotePort(socketId);

            LOG_ERR("Connect {0}:{1} failed", remoteIp, port);
        }
        break;
    case ec_net::eNET_SEND_OVERFLOW:
        LOG_ERR("Send buffer overflow! check \"BuffSize send\" size you've passed in \"conncet\" function on Lua script");
        break;
    case ec_net::eNET_PACKET_PARSE_FAILED:
        LOG_ERR("Packet parse failed! Packet size you returned from Lua is bigger then receive. check \"__APP_on_read_socket_buffer\" on Lua script");
        break;
    default:;
    }

    for (auto& [key, client] : m_mapClients) {
        if (client->getSocketID() == socketId) {
            client->onError(socketId, error);
            break;
        }
    }

    if (ui.cbClientName->count() == 0) {
        connectStateChange(EConnectState::kDisconnect);
    }
}

void CMainWindow::handleListMessageItemDoubleClicked(QListWidgetItem* pItem) {
    if (nullptr == pItem) {
        return;
    }

    if (pItem->text().isEmpty()) {
        return;
    }

    google::protobuf::Message* pMessage = nullptr;
    int idx = ui.tabWidget->currentIndex();
    std::string msgFullName = pItem->data(Qt::UserRole + kMessageFullName).toString().toStdString();
    if (0 == idx) {
      pMessage = getOrCreateMessageByName(msgFullName.c_str());
    } else if (1 == idx) {
      int selectIdx = ui.listRecentMessage->currentRow();
      if (selectIdx != -1 && selectIdx < m_listRecentMessages.size()) {
          auto itR = m_listRecentMessages.rbegin();
          std::advance(itR, selectIdx);
          pMessage = *itR;
      }
    }

    if (nullptr == pMessage) {
        return;
    }

    QListWidget* pListWidget = ui.listMessage;
    if (pItem->listWidget() == ui.listMessage) {
        pListWidget = ui.listRecentMessage;
    }

    auto* pDlg = new CMsgEditorDialog(this);
    pDlg->initDialogByMessage(pMessage);

    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        pMessage->CopyFrom(pDlg->getMessage());
        std::string msgStr;
        google::protobuf::util::MessageToJsonString(*pMessage, &msgStr, ConfigHelper::instance().getJsonPrintOption());
        if (!msgStr.empty()) {
            pItem->setIcon(QIcon(":/WhiteBox/icon1.ico"));
            //pItem->setToolTip(msgStr.c_str());
            pItem->setData(Qt::UserRole + kMessageData, msgStr.c_str());

            QList<QListWidgetItem*> listItems = pListWidget->findItems(pItem->text(), Qt::MatchExactly);
            for (int i = 0; i < listItems.count(); ++i) {
                //listItems[i]->setToolTip(msgStr.c_str());
                listItems[i]->setData(Qt::UserRole + kMessageData, msgStr.c_str());
            }
        }
    }

    delete pDlg;

    if (pMessage->ByteSizeLong() == 0) {
        delete pMessage;
        m_mapMessages.erase(msgFullName);

        pItem->setIcon(QIcon());
        //pItem->setToolTip("");
        pItem->setData(Qt::UserRole + kMessageData, "");

        QList<QListWidgetItem*> listItems = pListWidget->findItems(pItem->text(), Qt::MatchExactly);
        for (int i = 0; i < listItems.count(); ++i) {
            //listItems[i]->setToolTip("");
            listItems[i]->setData(Qt::UserRole + kMessageData, "");
        }
    }

    ui.labelCacheCnt->setText(std::to_string(m_mapMessages.size()).c_str());
}

void CMainWindow::handleMouseEntered(QListWidgetItem* pItem) {
    QString msgFullName = pItem->data(Qt::UserRole).toString();
    std::string detail = pItem->data(Qt::UserRole + kMessageData).toString().toStdString();

    auto rect = ui.listMessage->visualItemRect(pItem);
    auto pt = ui.listMessage->mapToGlobal(rect.topRight());

    QToolTip::showText(pt, m_highlighter->highlightJsonData(detail.c_str()).c_str(), ui.listMessage);
}

void CMainWindow::handleListLogItemCurrentRowChanged(int row) {
    QListWidgetItem* pItem = ui.listLogs->item(row);
    if (nullptr == pItem) {
        return;
    }

    ui.plainTextEdit->clear();

    std::string detail = pItem->data(Qt::UserRole + kMessageData).toString().toStdString();
    if (detail.empty()) {
      return;
    }

    ui.plainTextEdit->setPlainText(detail.c_str());
    m_highlighter->hightlight();
    handleSearchDetailTextChanged();
}

void CMainWindow::handleListLogCustomContextMenuRequested(const QPoint& pos) {
    QListWidgetItem* pCurItem = ui.listLogs->itemAt(pos);
    if (nullptr == pCurItem) {
        return;
    }

    if (pCurItem->data(Qt::UserRole + kMessageFullName).toString().isEmpty()) {
        return;
    }

    QMenu* popMenu = new QMenu(this);
    QAction* pAddIgnore = new QAction(tr("Add to ignore list"), this);
    popMenu->addAction(pAddIgnore);
    connect(pAddIgnore, SIGNAL(triggered()), this, SLOT(handleListLogActionAddToIgnoreList()));
    popMenu->exec(QCursor::pos());
    delete popMenu;
    delete pAddIgnore;
}

void CMainWindow::handleListLogActionAddToIgnoreList() {
    QListWidgetItem* pCurItem = ui.listLogs->currentItem();
    if (nullptr == pCurItem) {
        return;
    }

    QString msgFullName = pCurItem->data(Qt::UserRole + kMessageFullName).toString();
    if (msgFullName.isEmpty()) {
        return;
    }

    m_setIgnoredReceiveMsgType.insert(msgFullName.toStdString());
    ui.lableIgnoreCnt->setText(std::to_string(m_setIgnoredReceiveMsgType.size()).c_str());
}

void CMainWindow::handleFilterTextChanged(const QString& text) {
    // 搜索栏搜索逻辑
    for (int i = 0; i < ui.listMessage->count(); ++i) {
        ui.listMessage->item(i)->setHidden(!text.isEmpty());
    }

    bool bIsNumberic = false;
    int msgTypeNum = text.toInt(&bIsNumberic);
    if (bIsNumberic) {
        const CProtoManager::MsgInfo* pMsgInfo = ProtoManager::instance().getMsgInfoByMsgType(msgTypeNum);
        if (nullptr == pMsgInfo) {
            return;
        }

        std::string strMsgName = pMsgInfo->msgName;
        QList<QListWidgetItem*> listFound = ui.listMessage->findItems(strMsgName.c_str(), Qt::MatchCaseSensitive);
        for (int i = 0; i < listFound.count(); ++i) {
            listFound[i]->setHidden(false);
        }
    } else {
        QString rexPattern = text;
        rexPattern = rexPattern.replace(" ", ".*");
        rexPattern.prepend(".*");
        rexPattern.append(".*");
        QList<QListWidgetItem*> listFound = ui.listMessage->findItems(rexPattern, Qt::MatchRegExp);
        for (int i = 0; i < listFound.count(); ++i) {
            listFound[i]->setHidden(false);
        }
    }
}

void CMainWindow::handleSendBtnClicked() {
    QString clientName = ui.cbClientName->currentData().toString();
    if (clientName.isEmpty()) {
        QMessageBox::warning(nullptr, "", "Please select a client first");
        return;
    }

    lua_api::IClient* pClient = getClient(clientName.toStdString().c_str());
    if (nullptr == pClient) {
        QMessageBox::warning(nullptr, "", fmt::format("Cannot find client name: {}", clientName.toStdString()).c_str());
        return;
    }

    if (!pClient->isConnected()) {
        QMessageBox::warning(nullptr, "", "Please connect to server");
        return;
    }

    google::protobuf::Message* pMessage = nullptr;
    int idx = ui.tabWidget->currentIndex();
    QListWidgetItem* pSelectItem = nullptr;
    if (0 == idx) {
        pSelectItem = ui.listMessage->currentItem();
    } else if (1 == idx) {
        pSelectItem = ui.listRecentMessage->currentItem();
    }

    if (nullptr == pSelectItem) {
        QMessageBox::warning(nullptr, "", "Please select a protocol message to send");
        return;
    }

    QString selectMsgName = pSelectItem->text();
    std::string msgFullName = pSelectItem->data(Qt::UserRole + kMessageFullName).toString().toStdString();

    if (0 == idx) {
        pMessage = getOrCreateMessageByName(msgFullName.c_str());
    } else if (1 == idx) {
        int selectIdx = ui.listRecentMessage->currentRow();
        if (selectIdx != -1 && selectIdx < m_listRecentMessages.size()) {
          auto itR = m_listRecentMessages.rbegin();
          std::advance(itR, selectIdx);
          pMessage = *itR;
        }
    }

    if (nullptr != pMessage) {
        static_cast<CClient*>(pClient)->sendMsg(*pMessage);
        std::string msgStr;
        google::protobuf::util::MessageToJsonString(*pMessage, &msgStr, ConfigHelper::instance().getJsonPrintOption());
        addDetailLogInfo(msgFullName.c_str()
                         , fmt::format("Sent Message <font color='{0}'>{1}</font>", "#AF7AC5", selectMsgName.toStdString())
                         , msgStr.c_str());

        if (0 == idx) {
            auto* pItem = new QListWidgetItem(selectMsgName.append(" [")
                              .append(std::to_string(ui.listRecentMessage->count() + 1).c_str())
                              .append("]"));
            //pItem->setToolTip(msgStr.c_str());
            pItem->setData(Qt::UserRole + kMessageData, msgStr.c_str());
            pItem->setData(Qt::UserRole + kMessageFullName, msgFullName.c_str());
            auto* pRecentMsg = ProtoManager::instance().createMessage(msgFullName.c_str());
            pRecentMsg->CopyFrom(*pMessage);
            m_listRecentMessages.emplace_back(pRecentMsg);
            ui.listRecentMessage->insertItem(0, pItem);

            if (ui.listRecentMessage->count() > 100) {
                delete ui.listRecentMessage->takeItem(ui.listRecentMessage->count());
            }
        }
    }
}

void CMainWindow::handleConnectBtnClicked() {
    // 如果点击的时断开连接
    if (m_btnConnectState == EConnectState::kConnected) {
        for (auto& [key, client] : m_mapClients) {
            client->disconnect();
        }
        connectStateChange(EConnectState::kDisconnect);
        return;
    }

    // 判断是不是新的ip和端口，如果是，添加历史
    addNewItemIntoCombox(*ui.cbIp);

    QStringList ipAndPort = ui.cbIp->currentText().split(":");
    if (ipAndPort.size() != 2) {
        QMessageBox::warning(this, "", "IP and Port not correct. it should like: 192.168.1.1:28888");
        return;
    }

    if (ui.cbAccount->currentText().isEmpty()) {
        QMessageBox::warning(this, "", "Account cannot be empty");
        return;
    }

    addNewItemIntoCombox(*ui.cbAccount);

    if (!ui.cbOptionalParam->currentText().isEmpty()) {
        addNewItemIntoCombox(*ui.cbOptionalParam);
    }

    std::string ip = ipAndPort[0].toStdString();
    Port port = ipAndPort[1].toUShort();

    LuaScriptSystem::instance().Invoke("__APP_on_connect_btn_click"
                                        , ip
                                        , port
                                        , ui.cbAccount->currentText().toStdString().c_str()
                                        , ui.cbOptionalParam->currentText().toStdString().c_str());

    connectStateChange(EConnectState::kConnecting);
    LOG_INFO("Connecting to {}:{}...", ip, port);

    setWindowTitle(fmt::format("{}:{}", WINDOWS_TITLE, ui.cbAccount->currentText().toStdString()).c_str());
}

void CMainWindow::handleSearchDetailTextChanged() {
    // 详情搜索栏搜索逻辑
    QString searchString = ui.editSearchDetail->text();
    QTextDocument* document = ui.plainTextEdit->document();

    if (m_vecSearchPos.size() != 0) {
        // 需要undo两次
        document->undo();
        document->undo();
    }

    m_vecSearchPos.clear();
    m_searchResultIdx = -1;

    if (document->isEmpty()) {
        return;
    }

    int pos = 0;
    if (!searchString.isEmpty()) {
        QTextCursor highlightCursor(document);
        QTextCursor cursor(document);

        cursor.beginEditBlock();

        QTextCharFormat plainFormat(highlightCursor.charFormat());
        QTextCharFormat colorFormat = plainFormat;
        colorFormat.setBackground(QColor(255, 195, 0));

        while (!highlightCursor.isNull() && !highlightCursor.atEnd()) {
            highlightCursor = document->find(searchString, highlightCursor);

            if (!highlightCursor.isNull()) {
                highlightCursor.mergeCharFormat(colorFormat);
                m_vecSearchPos.emplace_back(highlightCursor.position());
            }
        }

        cursor.endEditBlock();

        if (!m_vecSearchPos.empty()) {
            pos = m_vecSearchPos[0];
        }

        // 高亮第一个搜索结果
        cursor = ui.plainTextEdit->textCursor();
        cursor.beginEditBlock();
        cursor.setPosition(pos);
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, searchString.length());
        QTextCharFormat textFormat(cursor.charFormat());
        textFormat.setBackground(QColor(255, 150, 50));
        cursor.mergeCharFormat(textFormat);
        cursor.setPosition(pos);
        cursor.endEditBlock();
        cursor.setCharFormat(plainFormat);
        ui.plainTextEdit->setTextCursor(cursor);

        if (!m_vecSearchPos.empty()) {
            m_searchResultIdx = 0;
        }
    }

    ui.labelSearchCnt->setText(fmt::format("{}/{}", m_searchResultIdx + 1, m_vecSearchPos.size()).c_str());
};

void CMainWindow::handleSearchDetailBtnLastResult() {
    if (-1 == m_searchResultIdx) {
        return;
    }

    if (0 == m_searchResultIdx) {
        m_searchResultIdx = m_vecSearchPos.size();
    }

    if (m_vecSearchPos.size() != 0) {
        ui.plainTextEdit->document()->undo();
    }

    m_searchResultIdx--;

    int pos = m_vecSearchPos[m_searchResultIdx];

    QTextCharFormat plainFormat(ui.plainTextEdit->textCursor().charFormat());
    QTextCursor cursor = ui.plainTextEdit->textCursor();
    cursor.beginEditBlock();
    cursor.setPosition(pos);
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, ui.editSearchDetail->text().length());
    QTextCharFormat textFormat(cursor.charFormat());
    textFormat.setBackground(QColor(255, 150, 50));
    cursor.mergeCharFormat(textFormat);
    cursor.setPosition(pos);
    cursor.endEditBlock();
    cursor.setCharFormat(plainFormat);

    ui.plainTextEdit->setTextCursor(cursor);
    ui.labelSearchCnt->setText(fmt::format("{}/{}", m_searchResultIdx + 1, m_vecSearchPos.size()).c_str());
}

void CMainWindow::handleSearchDetailBtnNextResult() {
    if (-1 == m_searchResultIdx) {
        return;
    }

    if (m_searchResultIdx >= m_vecSearchPos.size() - 1) {
        m_searchResultIdx = 0;
    } else {
        m_searchResultIdx++;
    }

    int pos = m_vecSearchPos[m_searchResultIdx];

    if (!m_vecSearchPos.empty()) {
        ui.plainTextEdit->document()->undo();
    }

    QTextCharFormat plainFormat(ui.plainTextEdit->textCursor().charFormat());
    QTextCursor cursor = ui.plainTextEdit->textCursor();
    cursor.beginEditBlock();
    cursor.setPosition(pos);
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, ui.editSearchDetail->text().length());
    QTextCharFormat textFormat(cursor.charFormat());
    textFormat.setBackground(QColor(255, 150, 50));
    cursor.mergeCharFormat(textFormat);
    cursor.setPosition(pos);
    cursor.endEditBlock();
    cursor.setCharFormat(plainFormat);

    ui.plainTextEdit->setTextCursor(cursor);
    ui.labelSearchCnt->setText(fmt::format("{}/{}", m_searchResultIdx + 1, m_vecSearchPos.size()).c_str());
}

void CMainWindow::handleSearchLogTextChanged() {
    // 日志搜索栏搜索逻辑
    m_listFoundItems.clear();
    m_searchLogResultIdx = -1;
    QPalette p = qApp->palette();
    QColor defaultColor = p.color(QPalette::Base);
    QString searchString = ui.editLogSearch->text();
    if (searchString.isEmpty()) {
        for (int i = 0; i < ui.listLogs->count(); ++i) {
            QListWidgetItem* pItem = ui.listLogs->item(i);
            pItem->setBackground(defaultColor);
        }
    } else {
        m_listFoundItems = ui.listLogs->findItems(searchString, Qt::MatchContains);
        for (int i = 0; i < ui.listLogs->count(); ++i) {
            QListWidgetItem* pItem = ui.listLogs->item(i);
            if (m_listFoundItems.contains(pItem)) {
                pItem->setBackground(QColor(255, 195, 0));
            } else {
                pItem->setBackground(defaultColor);
            }
        }

        if (m_listFoundItems.count() != 0) {
            m_searchLogResultIdx = 0;
            int nCurRow = ui.listLogs->currentRow();

            for (int i = m_listFoundItems.size() - 1; i > 0; i--) {
                if (ui.listLogs->row(m_listFoundItems[i]) >= nCurRow) {
                    m_searchLogResultIdx = i;
                    break;
                }
            }

            ui.listLogs->scrollToItem(m_listFoundItems[m_searchLogResultIdx]);
            ui.listLogs->setCurrentItem(m_listFoundItems[m_searchLogResultIdx]);
            m_listFoundItems[m_searchLogResultIdx]->setBackground(QColor(255, 150, 50));
        } else {
            m_searchLogResultIdx = -1;
        }
    }

    ui.labelLogSearchCnt->setText(fmt::format("{}/{}", m_searchLogResultIdx + 1, m_listFoundItems.size()).c_str());
}

void CMainWindow::handleSearchLogBtnLastResult() {
    if (-1 == m_searchLogResultIdx) {
        return;
    }

    m_listFoundItems[m_searchLogResultIdx]->setBackground(QColor(255, 195, 0));
    if (0 == m_searchLogResultIdx) {
        m_searchLogResultIdx = m_listFoundItems.size();
    }

    m_searchLogResultIdx--;
    m_listFoundItems[m_searchLogResultIdx]->setBackground(QColor(255, 150, 50));
    ui.listLogs->scrollToItem(m_listFoundItems[m_searchLogResultIdx]);
    ui.listLogs->setCurrentItem(m_listFoundItems[m_searchLogResultIdx]);
    ui.labelLogSearchCnt->setText(fmt::format("{}/{}", m_searchLogResultIdx + 1, m_listFoundItems.size()).c_str());
}

void CMainWindow::handleSearchLogBtnNextResult() {
    if (-1 == m_searchLogResultIdx) {
        return;
    }

    m_listFoundItems[m_searchLogResultIdx]->setBackground(QColor(255, 195, 0));

    if (m_searchLogResultIdx + 1 >= m_listFoundItems.size()) {
        m_searchLogResultIdx = 0;
    } else {
        m_searchLogResultIdx++;
    }

    m_listFoundItems[m_searchLogResultIdx]->setBackground(QColor(255, 150, 50));
    ui.listLogs->scrollToItem(m_listFoundItems[m_searchLogResultIdx]);
    ui.listLogs->setCurrentItem(m_listFoundItems[m_searchLogResultIdx]);
    ui.labelLogSearchCnt->setText(fmt::format("{}/{}", m_searchLogResultIdx + 1, m_listFoundItems.size()).c_str());
}

void CMainWindow::handleBtnIgnoreMsgClicked() {
    auto* pDlg = new CMsgIgnoreDialog(this);
    pDlg->init(m_setIgnoredReceiveMsgType);
    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        m_setIgnoredReceiveMsgType.clear();
        const std::list<std::string>& listIgnoredMsg = pDlg->getIgnoredMsg();
        auto it = listIgnoredMsg.begin();
        for (; it != listIgnoredMsg.end(); ++it) {
            m_setIgnoredReceiveMsgType.insert(*it);
        }

        ui.lableIgnoreCnt->setText(std::to_string(m_setIgnoredReceiveMsgType.size()).c_str());
    }

    delete pDlg;
}

void CMainWindow::handleBtnClearLogClicked() {
    ui.listLogs->clear();
}

void CMainWindow::handleLogInfoAdded(const QModelIndex& parent, int start, int end) {
    handleSearchLogTextChanged();
    if (m_pLogUpdateTimer != nullptr) {
        if (ui.checkIsAutoDetail->isChecked()) {
            m_pLogUpdateTimer->start(40);
            m_pLogUpdateTimer->setSingleShot(true);
        }
    }
}

void CMainWindow::doReload() {
    m_pMaskWidget->setGeometry(0, 0, width(), height());
    m_pMaskWidget->show();

    for (auto& [key, client] : m_mapClients) {
        qApp->processEvents();
        client->disconnect();
    }

    // 关闭lua
    LuaScriptSystem::instance().Shutdown();

    clearProto();
    clearCache();

    LOG_INFO("Reloading lua script path: \"{}\"..."
             , ConfigHelper::instance().getWidgetComboxStateText("LuaScriptPath", 0).toStdString());
    qApp->processEvents();
    if (!loadLua()) {
        m_pMaskWidget->hide();
        return;
    }
    LOG_INFO("Reloading proto files from path:\"{}\"..."
             , ConfigHelper::instance().getWidgetComboxStateText("ProtoPath", 0).toStdString());
    qApp->processEvents();
    if (!loadProto()) {
        m_pMaskWidget->hide();
        return;
    }
    // 加载缓存 
    LOG_INFO("Reloading cache...");
    qApp->processEvents();
    loadCache();

    handleFilterTextChanged(ui.editSearch->text());
    qApp->processEvents();
    m_pMaskWidget->hide();
}

void CMainWindow::closeEvent(QCloseEvent* event) {
    ConfigHelper::instance().saveWidgetComboxState("Account", *ui.cbAccount);
    ConfigHelper::instance().saveWidgetComboxState("Ip_Port", *ui.cbIp);
    ConfigHelper::instance().saveWidgetComboxState("OptionalParam", *ui.cbOptionalParam);
    ConfigHelper::instance().saveMainWindowGeometry(saveGeometry());
    ConfigHelper::instance().saveMainWindowState(saveState());
    ConfigHelper::instance().saveSplitterH(ui.splitterH->saveState());
    ConfigHelper::instance().saveSplitterV(ui.splitterV->saveState());
    ConfigHelper::instance().saveWidgetCheckboxState("AutoShowDetail", *ui.checkIsAutoDetail);

    // 自动保存cache
    saveCache();
}

void CMainWindow::timerEvent(QTimerEvent* event) {
    LuaScriptSystem::instance().Invoke("__APP_on_timer", event->timerId());
}

void CMainWindow::keyPressEvent(QKeyEvent* event) {
    // 上一个搜索结果
    if (event->modifiers() == Qt::ShiftModifier && event->key() == Qt::Key::Key_F3) {
        if (ui.editSearchDetail->text().isEmpty()) {
            return;
        }

        handleSearchDetailBtnLastResult();
    } else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key::Key_F3) {
        // 搜索
        QTextCursor cursor = ui.plainTextEdit->textCursor();
        if (cursor.hasSelection()) {
            ui.editSearchDetail->setText(cursor.selectedText());
        }
    } else if (event->key() == Qt::Key::Key_F3) {
        // 下一个搜索结果
        if (ui.editSearchDetail->text().isEmpty()) {
            return;
        }

        handleSearchDetailBtnNextResult();
    }
}

void CMainWindow::addNewItemIntoCombox(QComboBox& combox) {
    int cbIdx = combox.findText(combox.currentText());
    if (-1 == cbIdx) {
        combox.insertItem(0, combox.currentText());
        if (combox.count() > ConfigHelper::instance().getHistroyComboxItemMaxCnt()) {
            combox.removeItem(combox.count() - 1);
        }
    } else {
        QString text = combox.currentText();
        combox.removeItem(cbIdx);
        combox.insertItem(0, text);
    }
    combox.setCurrentIndex(0);
}

bool CMainWindow::loadProto() {
    QString strPath = ConfigHelper::instance().getWidgetComboxStateText("ProtoPath", 0);
    QDir protoDir = strPath;
    ProtoManager::instance().init(protoDir.absolutePath().toStdString());
    LOG_INFO("Importing proto files... from {}", strPath.toStdString());

    bool bSuccess = false;
    QDirIterator dirIt(strPath, QStringList() << "*.proto", QDir::Files, QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        QString virtualPath = protoDir.relativeFilePath(dirIt.next());
        bSuccess |= ProtoManager::instance().importProto(virtualPath.toStdString());
    }

    if (!bSuccess) {
        LOG_ERR("Import proto failed, check the path is correct.\nProtoPath: {}", strPath.toStdString());
        return false;
    }

    LuaScriptSystem::instance().Invoke("__APP_on_proto_reload"
                                       , static_cast<lua_api::IProtoManager*>(&ProtoManager::instance()));

    // 填充消息列表
    std::list<CProtoManager::MsgInfo> listNames = ProtoManager::instance().getMsgInfos();
    auto it = listNames.begin();
    for (; it != listNames.end(); ++it) {
        auto* pListItem = new QListWidgetItem();
        pListItem->setText(it->msgName.c_str());
        pListItem->setData(Qt::UserRole + kMessageFullName, it->msgFullName.c_str());
        ui.listMessage->addItem(pListItem);
    }

    ui.labelMsgCnt->setText(std::to_string(ui.listMessage->count()).c_str());
    LOG_INFO("Import proto files completed");
    return true;
}

bool CMainWindow::loadLua() {
    LuaScriptSystem::instance().Setup();
    luaopen_protobuf(LuaScriptSystem::instance().GetLuaState());
    luaopen_json(LuaScriptSystem::instance().GetLuaState());
    luaRegisterCppClass();
    LOG_INFO("Initiated lua script system");

    QString luaScriptPath = ConfigHelper::instance().getWidgetComboxStateText("LuaScriptPath", 0);
    LOG_INFO("Loading lua script: \"{}\"", luaScriptPath.toStdString());
    if (!LuaScriptSystem::instance().RunScript(luaScriptPath.toStdString().c_str())) {
        LOG_ERR("Error: Load lua script: \"{}\" failed!", luaScriptPath.toStdString());
        return false;
    }
    LOG_INFO("Lua script: \"{}\" loaded", luaScriptPath.toStdString());
    return true;
}

void CMainWindow::clearProto() {
    ui.labelMsgCnt->setText("0");
    ui.listMessage->clear();
    ui.listRecentMessage->clear();
    ProtoManager::instance().clear();
}

void CMainWindow::luaRegisterCppClass() {
    auto* pLuaState = LuaScriptSystem::instance().GetLuaState();
    if (nullptr == pLuaState) {
        return;
    }
    using namespace  lua_api;
    luabridge::getGlobalNamespace(pLuaState)
        .beginClass<ISocketReader>("ISocketReader")
        .addFunction("readUint8", &ISocketReader::readUint8)
        .addFunction("readInt8", &ISocketReader::readInt8)
        .addFunction("readUint16", &ISocketReader::readUint16)
        .addFunction("readInt16", &ISocketReader::readInt16)
        .addFunction("readUint32", &ISocketReader::readUint32)
        .addFunction("readInt32", &ISocketReader::readInt32)
        .addFunction("getDataPtr", &ISocketReader::getDataPtr)
        .addFunction("bindMessage", &ISocketReader::bindMessage)
        .endClass();

    luabridge::getGlobalNamespace(pLuaState)
        .beginClass<ISocketWriter>("ISocketWriter")
        .addFunction("writeUint8", &ISocketWriter::writeUint8)
        .addFunction("writeUint8", &ISocketWriter::writeInt8)
        .addFunction("writeUint16", &ISocketWriter::writeUint16)
        .addFunction("writeInt16", &ISocketWriter::writeInt16)
        .addFunction("writeUint32", &ISocketWriter::writeUint32)
        .addFunction("writeInt32", &ISocketWriter::writeInt32)
        .addFunction("writeBinary", &ISocketWriter::writeBinary)
        .endClass();

    luabridge::getGlobalNamespace(pLuaState)
        .beginClass<IProtoManager>("IProtoManager")
        .addFunction("addProtoMessage", &IProtoManager::addProtoMessage)
        .addFunction("getDescriptorPool", &IProtoManager::getDescriptorPool)
        .endClass();

    luabridge::getGlobalNamespace(pLuaState)
        .beginClass<IClient>("IClient")
        .addFunction("connect", &IClient::connect)
        .addFunction("disconnect", &IClient::disconnect)
        .addFunction("isConnected", &IClient::isConnected)
        .addFunction("getName", &IClient::getName)
        .addFunction("sendJsonMsg", &IClient::sendJsonMsg)
        .endClass();

    luabridge::getGlobalNamespace(pLuaState)
        .beginClass<IMainApp>("IApp")
        .addFunction("addTimer", &IMainApp::addTimer)
        .addFunction("deleteTimer", &IMainApp::deleteTimer)
        .addFunction("createClient", &IMainApp::createClient)
        .addFunction("getClient", &IMainApp::getClient)
        .addFunction("log", &IMainApp::log)
        .addFunction("logErr", &IMainApp::logErr)
        .endClass();

    luabridge::setGlobal(pLuaState, static_cast<IMainApp*>(this), "App");
}

void CMainWindow::addDetailLogInfo(const char* msgFullName, const std::string& msg, const char* detail) {
    auto* pListWidgetItem = new QListWidgetItem;
    pListWidgetItem->setData(Qt::UserRole + kMessageData, QString(detail));
    pListWidgetItem->setData(Qt::UserRole + kMessageFullName, msgFullName);

    QDateTime localTime(QDateTime::currentDateTime());
    QString timeStr = "<font color='grey'>" + localTime.time().toString() + "</font> ";
    QLabel* l = new QLabel(timeStr + msg.c_str());
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(l);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->setContentsMargins(3, 2, 3, 2);
    QWidget* w = new QWidget;
    w->setLayout(layout);
    pListWidgetItem->setSizeHint(w->sizeHint());
    ui.listLogs->addItem(pListWidgetItem);
    ui.listLogs->setItemWidget(pListWidgetItem, w);
}
