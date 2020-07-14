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

const char WINDOWS_TITLE[25] = "EmuCl";

struct ECVersion
{
    int main;
    int sub;
    int build;
} const g_version = { 3 , 0 , 1 };

CMainWindow::CMainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_cbKeyPressFilter(new DeleteHighlightedItemFilter)
    , m_listMessageKeyFilter(new ShowItemDetailKeyFilter) {
    ui.setupUi(this);

    setWindowTitle(WINDOWS_TITLE);

    // 恢复上次的布局大小
    restoreGeometry(ConfigHelper::instance().getMainWindowGeometry());
    restoreState(ConfigHelper::instance().getMainWindowState());
    ui.splitterH->restoreState(ConfigHelper::instance().getSplitterH());
    ui.splitterV->restoreState(ConfigHelper::instance().getSplitterV());

    // 用户名
    {
        auto* pLineEdit = new CLineEdit(ui.cbAccount);
        pLineEdit->setPlaceholderText("Account");
        ui.cbAccount->setLineEdit(pLineEdit);
    }
    ui.cbAccount->setItemDelegate(new QStyledItemDelegate());
    ui.cbAccount->view()->installEventFilter(m_cbKeyPressFilter);
    ConfigHelper::instance().restoreWidgetComboxState("Account", *ui.cbAccount);

    // ip端口
    {
        auto* pLineEdit = new CLineEdit(ui.cbIp);
        auto* pAddrValidator = new IP4Validator(ui.cbIp);
        pLineEdit->setValidator(pAddrValidator);
        pLineEdit->setPlaceholderText("IP:Port");
        ui.cbIp->setLineEdit(pLineEdit);
    }
    ui.cbIp->setItemDelegate(new QStyledItemDelegate());
    ui.cbIp->view()->installEventFilter(m_cbKeyPressFilter);
    ConfigHelper::instance().restoreWidgetComboxState("Ip_Port", *ui.cbIp);

    // 可选项参数
    {
        auto* pLineEdit = new CLineEdit(ui.cbIp);
        pLineEdit->setPlaceholderText("Optional");
        ui.cbOptionalParam->setLineEdit(pLineEdit);
    }
    ui.cbOptionalParam->setItemDelegate(new QStyledItemDelegate());
    ui.cbOptionalParam->view()->installEventFilter(m_cbKeyPressFilter);
    ConfigHelper::instance().restoreWidgetComboxState("OptionalParam", *ui.cbOptionalParam);

    ui.cbClientName->setItemDelegate(new QStyledItemDelegate());

    // 读取是否自动显示最新
    ConfigHelper::instance().restoreWidgetCheckboxState("AutoShowDetail", *ui.checkIsAutoDetail);

    ui.listMessage->installEventFilter(m_listMessageKeyFilter);
    ui.listRecentMessage->installEventFilter(m_listMessageKeyFilter);

    ui.editSearchDetail->setStyleSheet(ConfigHelper::instance().getStyleSheetLineEditNormal());
    ui.editSearch->setStyleSheet(ConfigHelper::instance().getStyleSheetLineEditNormal());
    ui.editLogSearch->setStyleSheet(ConfigHelper::instance().getStyleSheetLineEditNormal());

    // 给log窗口添加右键菜单
    ui.listLogs->setContextMenuPolicy(Qt::CustomContextMenu);

    ui.btnSend->setDisabled(true);

    QObject::connect(ui.listMessage, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(handleListMessageItemDoubleClicked(QListWidgetItem*)));

    QObject::connect(ui.listRecentMessage, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(handleListMessageItemDoubleClicked(QListWidgetItem*)));
    QObject::connect(ui.listRecentMessage, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(handleListMessageCurrentItemChanged(QListWidgetItem*, QListWidgetItem*)));
    QObject::connect(ui.listRecentMessage, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(handleListMessageItemClicked(QListWidgetItem*)));

    QObject::connect(ui.listLogs, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(handleListLogItemCurrentItemChanged(QListWidgetItem*, QListWidgetItem*)));
    QObject::connect(ui.listLogs, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(handleListLogItemClicked(QListWidgetItem*)));
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
}

CMainWindow::~CMainWindow() {
    delete m_cbKeyPressFilter;
    delete m_listMessageKeyFilter;
    delete m_pPrinter;

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

    // 启动主定时器
    auto* pMainTimer = new QTimer(this);
    connect(pMainTimer, &QTimer::timeout, this, QOverload<>::of(&CMainWindow::update));
    pMainTimer->start(40);

    LOG_INFO("Ready.             - Version {}.{}.{} By marskey.", g_version.main, g_version.sub, g_version.build);
    return true;
}

void CMainWindow::connectStateChange(EConnectState state) {
    switch (state) {
    case kConnected:
        ui.btnConnect->setText("Disconnect");
        ui.btnConnect->setDisabled(false);
        ui.btnSend->setDisabled(false);

        ui.cbIp->setDisabled(true);
        ui.cbAccount->setDisabled(true);
        ui.cbOptionalParam->setDisabled(true);
        break;
    case kDisconnect:
        ui.btnConnect->setText("Connect");
        ui.btnConnect->setDisabled(false);
        ui.btnSend->setDisabled(true);

        ui.cbIp->setDisabled(false);
        ui.cbAccount->setDisabled(false);
        ui.cbOptionalParam->setDisabled(false);
        break;
    case kConnecting:
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
    LOG_INFO(message);
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
        json["data"] = msgData.c_str();
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
    for (QJsonValueRef& cache : msgCacheArray) {
        QJsonObject obj = cache.toObject();
        std::string msgName = obj["name"].toString().toStdString();
        google::protobuf::Message* pMessage = getOrCreateMessageByName(msgName.c_str());
        if (nullptr == pMessage) {
            LOG_ERR("Error: Cache file {} is corrupted!", path);
            ++msgCacheFailedCnt;
            continue;
        }

        if (pMessage->ParseFromString(obj["data"].toString().toStdString())) {
            m_mapMessages[msgName] = pMessage;

            std::string msgStr;
            for (int i = 0; i < ui.listMessage->count(); ++i) {
                QListWidgetItem* pItem = ui.listMessage->item(i);
                if (nullptr != pItem) {
                    std::string msgFullName = pItem->data(Qt::UserRole).toString().toStdString();
                    if (msgFullName == msgName) {
                        pItem->setIcon(QIcon(":/EmulatorClient/icon1.ico"));
                        google::protobuf::util::MessageToJsonString(*pMessage, &msgStr, ConfigHelper::instance().getJsonPrintOption());
                        pItem->setToolTip(msgStr.c_str());
                    }
                }
            }
        }
    }
    ui.labelCacheCnt->setText(std::to_string(m_mapMessages.size()).c_str());


    int ignoreMsgFailedCnt = 0;
    QJsonArray ignoredMsgArray = jsonObj["ignoredMsg"].toArray();
    for (QJsonValueRef& cache : ignoredMsgArray) {
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
            pListWidgetItem->setToolTip("");
        }
    }
    ui.labelCacheCnt->setText("0");

    for (int i = 0; i < ui.listRecentMessage->count(); ++i) {
        QListWidgetItem* pListWidgetItem = ui.listRecentMessage->item(i);
        if (nullptr != pListWidgetItem) {
            pListWidgetItem->setToolTip("");
        }
    }
    //QMessageBox::information(this, "", fmt::format("Cleared {0} messages.", count).c_str());
}

void CMainWindow::update() {
    NetManager::instance().run();

    for (auto& client : m_listClientsToDel) {
        m_mapClients.erase(client->getName());
        delete client;
    }
    m_listClientsToDel.clear();
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

std::string CMainWindow::highlightJsonData(const QString& jsonData) {
    QString result = jsonData;
    QRegularExpression reAll;
    reAll.setPattern("(\"(\\\\u[a-zA-Z0-9]{4}|\\\\[^u]|[^\"])*\"(\\s*:)?|\\b(true|false)\\b|-?\\d+(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?)");
    QRegularExpressionMatchIterator i = reAll.globalMatch(jsonData);

    int offset = 0;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString word = match.captured();

        QString style = "color: #d4a956;"; // number

        QRegularExpression re;
        re.setPattern("^\"");
        QRegularExpressionMatch subMatch = re.match(word);

        re.setPattern("true|false");
        QRegularExpressionMatch subMatch2 = re.match(word);

        if (subMatch.hasMatch()) {
            re.setPattern(":$");
            subMatch = re.match(word);
            if (subMatch.hasMatch()) {
                style = "color: #bf3a42;"; // key
            } else {
                style = "color: #256d05;"; // string
            }
        } else if (subMatch2.hasMatch()) {
            style = "color: #7ebdff"; // bool
        }

        QString newWord = "<span style=\"" + style + "\">" + word + "</span>";
        result.replace(match.capturedStart() + offset, match.capturedLength(), newWord);
        offset += newWord.length() - match.capturedLength();
    }

    return result.toStdString();
}

void CMainWindow::onParseMessage(SocketId socketId, const char* msgFullName, const char* pData, size_t size) {
    if (msgFullName == nullptr) {
        LOG_ERR("Message name is nullptr, check lua script's function \"bindMessage\"", msgFullName);
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
                             , fmt::format("Received {}({}) size: {}"
                                           , msgFullName
                                           , nMessageId
                                           , pRecvMesage->ByteSize()).c_str()
                             , highlightJsonData(msgStr.c_str()).c_str()
                             , QColor(57, 115, 157)
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
    connectStateChange(kConnected);

    for (auto& [key, client] : m_mapClients) {
        if (client->getSocketID() == socketId) {
            ui.cbClientName->addItem(fmt::format("[socket:{}] {}", socketId, client->getName()).c_str()
                                     , client->getName());
            client->onConnectSucceed(remoteIp, port, socketId);
            break;
        }
    }
}

void CMainWindow::onDisconnect(SocketId socketId) {
    LOG_INFO("Connection closed, socket id: {}", socketId);
    bool bHasConnect = false;
    for (auto& [key, client] : m_mapClients) {
        if (client->getSocketID() == socketId) {
            client->onDisconnect(socketId);
            m_listClientsToDel.emplace_back(client);
            int cbIdx = ui.cbClientName->findData(client->getName());
            if (-1 != cbIdx) {
                ui.cbClientName->removeItem(cbIdx);
            }
        }
        bHasConnect |= client->isConnected();
    }

    if (!bHasConnect) {
        connectStateChange(kDisconnect);
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
        LOG_ERR("Send buffer overflow!");
        break;
    default:;
    }

    bool bHasConnect = false;
    for (auto& [key, client] : m_mapClients) {
        if (client->getSocketID() == socketId) {
            client->onError(socketId, error);
            break;
        }
        bHasConnect |= client->isConnected();
    }

    if (!bHasConnect) {
        connectStateChange(kDisconnect);
    }
}

void CMainWindow::handleListMessageItemDoubleClicked(QListWidgetItem* pItem) {
    if (nullptr == pItem) {
        return;
    }

    if (pItem->text().isEmpty()) {
        return;
    }

    std::string msgFullName = pItem->data(Qt::UserRole).toString().toStdString();

    google::protobuf::Message* pMessage = getOrCreateMessageByName(msgFullName.c_str());
    if (nullptr == pMessage) {
        return;
    }

    QListWidget* pListWidget = ui.listMessage;
    if (pItem->listWidget() == ui.listMessage) {
        pListWidget = ui.listRecentMessage;
    }

    auto* pDlg = new CMsgEditorDialog(this);
    pDlg->restoreGeometry(ConfigHelper::instance().getSubWindowGeometry());
    pDlg->initDialogByMessage(pMessage);

    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        pMessage->CopyFrom(pDlg->getMessage());
        std::string msgStr;
        google::protobuf::util::MessageToJsonString(*pMessage, &msgStr, ConfigHelper::instance().getJsonPrintOption());
        if (!msgStr.empty()) {
            pItem->setIcon(QIcon(":/EmulatorClient/icon1.ico"));
            pItem->setToolTip(msgStr.c_str());

            QList<QListWidgetItem*> listItems = pListWidget->findItems(pItem->text(), Qt::MatchExactly);
            for (int i = 0; i < listItems.count(); ++i) {
                listItems[i]->setToolTip(msgStr.c_str());
            }
        }
    }

    ConfigHelper::instance().saveSubWindowGeometry(pDlg->saveGeometry());
    delete pDlg;

    if (pMessage->ByteSize() == 0) {
        delete pMessage;
        m_mapMessages.erase(msgFullName);

        pItem->setIcon(QIcon());
        pItem->setToolTip("");

        QList<QListWidgetItem*> listItems = pListWidget->findItems(pItem->text(), Qt::MatchExactly);
        for (int i = 0; i < listItems.count(); ++i) {
            listItems[i]->setToolTip("");
        }
    }

    ui.labelCacheCnt->setText(std::to_string(m_mapMessages.size()).c_str());
}

void CMainWindow::handleListMessageCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous) {
    ui.textEdit->clear();

    QListWidgetItem* pItem = current;
    if (nullptr == pItem) {
        return;
    }

    if (pItem->text().isEmpty()) {
        return;
    }

    std::string msgFullName = pItem->data(Qt::UserRole).toString().toStdString();

    google::protobuf::Message* pMessage = getMessageByName(msgFullName.c_str());
    if (nullptr == pMessage) {
        return;
    }

    ui.textEdit->setHtml(fmt::format("<!DOCTYPE html><html><body>{}<hr><pre>{}</pre></body></html>", pItem->text().toStdString(), highlightJsonData(pItem->toolTip())).c_str());
    handleSearchDetailTextChanged();
}

void CMainWindow::handleListMessageItemClicked(QListWidgetItem* pItem) {
    handleListMessageCurrentItemChanged(pItem, nullptr);
}

void CMainWindow::handleListLogItemCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous) {
    QListWidgetItem* pItem = current;
    if (nullptr == pItem) {
        return;
    }

    if (pItem->text().isEmpty()) {
        return;
    }

    ui.textEdit->clear();

    std::string detail = pItem->data(Qt::UserRole).toString().toStdString();
    ui.textEdit->setHtml(fmt::format("<!DOCTYPE html><html><body>{}<hr><pre>{}</pre></body></html>", pItem->text().toStdString(), detail).c_str());
    handleSearchDetailTextChanged();
}

void CMainWindow::handleListLogItemClicked(QListWidgetItem* pItem) {
    handleListLogItemCurrentItemChanged(pItem, nullptr);
}

void CMainWindow::handleListLogCustomContextMenuRequested(const QPoint& pos) {
    QListWidgetItem* pCurItem = ui.listLogs->itemAt(pos);
    if (nullptr == pCurItem) {
        return;
    }

    if (pCurItem->data(Qt::UserRole).toString().isEmpty()) {
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

    QString msgFullName = pCurItem->data(Qt::UserRole + 1).toString();
    if (msgFullName.isEmpty()) {
        return;
    }

    m_setIgnoredReceiveMsgType.insert(msgFullName.toStdString());
    ui.lableIgnoreCnt->setText(std::to_string(m_setIgnoredReceiveMsgType.size()).c_str());
}

void CMainWindow::handleFilterTextChanged(const QString& text) {
    // 搜索栏搜索逻辑
    ui.editSearch->setStyleSheet(ConfigHelper::instance().getStyleSheetLineEditError());
    for (int i = 0; i < ui.listMessage->count(); ++i) {
        ui.listMessage->item(i)->setHidden(!text.isEmpty());
    }

    bool found = false;
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
        found = !listFound.isEmpty();
    } else {
        QString rexPattern = text;
        rexPattern = rexPattern.replace(" ", ".*");
        rexPattern.prepend(".*");
        rexPattern.append(".*");
        QList<QListWidgetItem*> listFound = ui.listMessage->findItems(rexPattern, Qt::MatchRegExp);
        for (int i = 0; i < listFound.count(); ++i) {
            listFound[i]->setHidden(false);
        }
        found = !listFound.isEmpty();
    }

    if (found) {
        ui.editSearch->setStyleSheet(ConfigHelper::instance().getStyleSheetLineEditNormal());
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
    std::string msgFullName = pSelectItem->data(Qt::UserRole).toString().toStdString();

    google::protobuf::Message* pMessage = getOrCreateMessageByName(msgFullName.c_str());
    if (nullptr != pMessage) {
        static_cast<CClient*>(pClient)->sendMsg(*pMessage);
        std::string msgStr;
        google::protobuf::util::MessageToJsonString(*pMessage, &msgStr, ConfigHelper::instance().getJsonPrintOption());
        addDetailLogInfo(msgFullName.c_str()
                         , fmt::format("Sent Message {}", selectMsgName.toStdString()).c_str()
                         , highlightJsonData(msgStr.c_str()).c_str(), Qt::black);

        if (0 == ui.listRecentMessage->findItems(selectMsgName, Qt::MatchExactly).count()) {
            auto* pItem = new QListWidgetItem(selectMsgName, ui.listRecentMessage);
            pItem->setToolTip(msgStr.c_str());
            ui.listRecentMessage->insertItem(0, pItem);
        }

        if (ui.listRecentMessage->count() > 50) {
            delete ui.listRecentMessage->takeItem(ui.listRecentMessage->count());
        }
    }
}

void CMainWindow::handleConnectBtnClicked() {
    // 如果点击的时断开连接
    if (m_btnConnectState == kConnected) {
        for (auto& [key, client] : m_mapClients) {
            client->disconnect();
        }
        connectStateChange(kDisconnect);
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

    connectStateChange(kConnecting);
    LOG_INFO("Connecting to {}:{}...", ip, port);

    setWindowTitle(fmt::format("{}:{}", WINDOWS_TITLE, ui.cbAccount->currentText().toStdString()).c_str());
}

void CMainWindow::handleSearchDetailTextChanged() {
    // 详情搜索栏搜索逻辑
    QString searchString = ui.editSearchDetail->text();
    QTextDocument* document = ui.textEdit->document();

    // 需要undo两次
    document->undo();
    document->undo();

    m_vecSearchPos.clear();
    m_searchResultIdx = -1;
    ui.editSearchDetail->setStyleSheet(ConfigHelper::instance().getStyleSheetLineEditNormal());

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
        colorFormat.setBackground(Qt::yellow);

        while (!highlightCursor.isNull() && !highlightCursor.atEnd()) {
            highlightCursor = document->find(searchString, highlightCursor);

            if (!highlightCursor.isNull()) {
                highlightCursor.mergeCharFormat(colorFormat);
                if (m_vecSearchPos.empty()) {
                    pos = highlightCursor.position();
                }
                m_vecSearchPos.emplace_back(highlightCursor.position());
            }
        }

        cursor.endEditBlock();

        // 高亮第一个搜索结果
        cursor = ui.textEdit->textCursor();
        cursor.beginEditBlock();
        cursor.setPosition(pos);
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, searchString.length());
        QTextCharFormat textFormat(cursor.charFormat());
        textFormat.setBackground(QColor(255, 150, 50));
        cursor.mergeCharFormat(textFormat);
        cursor.setPosition(pos);
        cursor.endEditBlock();
        ui.textEdit->setTextCursor(cursor);

        if (!m_vecSearchPos.empty()) {
            m_searchResultIdx = 0;
        } else {
            ui.editSearchDetail->setStyleSheet(ConfigHelper::instance().getStyleSheetLineEditError());
        }
    }

    ui.labelSearchCnt->setText(fmt::format("{}/{}", m_searchResultIdx + 1, m_vecSearchPos.size()).c_str());
}

void CMainWindow::handleSearchDetailBtnLastResult() {
    if (-1 == m_searchResultIdx) {
        return;
    }

    if (0 == m_searchResultIdx) {
        m_searchResultIdx = m_vecSearchPos.size();
    }

    ui.textEdit->document()->undo();

    m_searchResultIdx--;

    int pos = m_vecSearchPos[m_searchResultIdx];

    QTextCursor cursor = ui.textEdit->textCursor();
    cursor.beginEditBlock();
    cursor.setPosition(pos);
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, ui.editSearchDetail->text().length());
    QTextCharFormat textFormat(cursor.charFormat());
    textFormat.setBackground(QColor(255, 150, 50));
    cursor.mergeCharFormat(textFormat);
    cursor.setPosition(pos);
    cursor.endEditBlock();

    ui.textEdit->setTextCursor(cursor);
    ui.labelSearchCnt->setText(fmt::format("{}/{}", m_searchResultIdx + 1, m_vecSearchPos.size()).c_str());
}

void CMainWindow::handleSearchDetailBtnNextResult() {
    if (-1 == m_searchResultIdx) {
        return;
    }

    if (m_vecSearchPos.size() > 1 + m_searchResultIdx) {
        m_searchResultIdx = 0;
    } else {
        m_searchResultIdx++;
    }

    int pos = m_vecSearchPos[m_searchResultIdx];

    ui.textEdit->document()->undo();
    QTextCursor cursor = ui.textEdit->textCursor();
    cursor.beginEditBlock();
    cursor.setPosition(pos);
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, ui.editSearchDetail->text().length());
    QTextCharFormat textFormat(cursor.charFormat());
    textFormat.setBackground(QColor(255, 150, 50));
    cursor.mergeCharFormat(textFormat);
    cursor.setPosition(pos);
    cursor.endEditBlock();

    ui.textEdit->setTextCursor(cursor);
    ui.labelSearchCnt->setText(fmt::format("{}/{}", m_searchResultIdx + 1, m_vecSearchPos.size()).c_str());
}

void CMainWindow::handleSearchLogTextChanged() {
    // 日志搜索栏搜索逻辑
    m_listFoundItems.clear();
    m_searchLogResultIdx = -1;
    QString searchString = ui.editLogSearch->text();
    ui.editLogSearch->setStyleSheet(ConfigHelper::instance().getStyleSheetLineEditNormal());
    if (searchString.isEmpty()) {
        for (int i = 0; i < ui.listLogs->count(); ++i) {
            QListWidgetItem* pItem = ui.listLogs->item(i);
            pItem->setBackground(Qt::white);
        }
    } else {
        m_listFoundItems = ui.listLogs->findItems(searchString, Qt::MatchContains);
        for (int i = 0; i < ui.listLogs->count(); ++i) {
            QListWidgetItem* pItem = ui.listLogs->item(i);
            if (m_listFoundItems.contains(pItem)) {
                pItem->setBackground(Qt::yellow);
            } else {
                pItem->setBackground(Qt::white);
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
            ui.editLogSearch->setStyleSheet(ConfigHelper::instance().getStyleSheetLineEditError());
        }
    }

    ui.labelLogSearchCnt->setText(fmt::format("{}/{}", m_searchLogResultIdx + 1, m_listFoundItems.size()).c_str());
}

void CMainWindow::handleSearchLogBtnLastResult() {
    if (-1 == m_searchLogResultIdx) {
        return;
    }

    m_listFoundItems[m_searchLogResultIdx]->setBackground(Qt::yellow);
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

    m_listFoundItems[m_searchLogResultIdx]->setBackground(Qt::yellow);

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
        auto& it = listIgnoredMsg.begin();
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

    if (ui.checkIsAutoDetail->isChecked()) {
        ui.listLogs->setCurrentRow(end);
    }
}

void CMainWindow::doReload() {
    for (auto& [key, client] : m_mapClients) {
        client->disconnect();
    }

    // 关闭lua
    LuaScriptSystem::instance().Shutdown();

    clearProto();
    clearCache();

    LOG_INFO("Reloading lua script path: \"{}\"..."
             , ConfigHelper::instance().getLuaScriptPath().toStdString());
    if (!loadLua()) {
        return;
    }
    LOG_INFO("Reloading proto files from path:\"{}\"..."
             , ConfigHelper::instance().getProtoPath().toStdString());
    if (!loadProto()) {
        return;
    }
    // 加载缓存 
    LOG_INFO("Reloading cache...");
    loadCache();
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
        QTextCursor cursor = ui.textEdit->textCursor();
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
    LOG_INFO("Importing proto files...");
    QString strPath = ConfigHelper::instance().getProtoPath();
    QDir protoDir = strPath;
    ProtoManager::instance().init(protoDir.absolutePath().toStdString());

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
        auto* pListItem = new QListWidgetItem(ui.listMessage);
        pListItem->setText(it->msgName.c_str());
        pListItem->setData(Qt::UserRole, it->msgFullName.c_str());
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

    QString luaScriptPath = ConfigHelper::instance().getLuaScriptPath();
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
        .endClass();

    luabridge::setGlobal(pLuaState, static_cast<IMainApp*>(this), "App");
}

void CMainWindow::addDetailLogInfo(const char* msgFullName, const char* msg, const char* detail, QColor color /*= QColor(Qt::GlobalColor(0))*/) {
    time_t t = time(nullptr);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "[%X] ", localtime(&t));

    std::string data = tmp;
    data.append(msg);
    auto* pListWidgetItem = new QListWidgetItem(data.c_str(), ui.listLogs);
    pListWidgetItem->setData(Qt::UserRole, detail);
    pListWidgetItem->setData(Qt::UserRole + 1, msgFullName);

    if (QColor(Qt::GlobalColor(0)) != color) {
        pListWidgetItem->setForeground(color);
    }

    ui.listLogs->addItem(pListWidgetItem);
}
