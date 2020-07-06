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
#include "Session.h"

extern "C" {
#include "protobuf/protobuflib.h"
}

#include <QTime>
#include <QTimer>
#include <QMessageBox>
#include <fstream>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDirIterator>

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
    restoreGeometry(ConfigHelper::singleton().getMainWindowGeometry());
    restoreState(ConfigHelper::singleton().getMainWindowState());
    ui.splitterH->restoreState(ConfigHelper::singleton().getSplitterH());
    ui.splitterV->restoreState(ConfigHelper::singleton().getSplitterV());

    {
        auto* pLineEdit = new CLineEdit(this);
        pLineEdit->setPlaceholderText("Account");
        ui.cbAccount->setLineEdit(pLineEdit);
    }
    ui.cbAccount->view()->installEventFilter(m_cbKeyPressFilter);

    // 读取用户名
    ConfigHelper::singleton().restoreWidgetComboxState("Account", *ui.cbAccount);

    {
        auto* pLineEdit = new CLineEdit(this);
        auto* pAddrValidator = new IP4Validator(this);
        pLineEdit->setValidator(pAddrValidator);
        pLineEdit->setPlaceholderText("IP:Port");
        ui.cbIp->setLineEdit(pLineEdit);
    }
    ui.cbIp->view()->installEventFilter(m_cbKeyPressFilter);

    // 读取ip端口
    ConfigHelper::singleton().restoreWidgetComboxState("Ip_Port", *ui.cbIp);

    // 读取是否自动显示最新
    ConfigHelper::singleton().restoreWidgetCheckboxState("AutoShowDetail", *ui.checkIsAutoDetail);

    ui.listMessage->installEventFilter(m_listMessageKeyFilter);
    ui.listRecentMessage->installEventFilter(m_listMessageKeyFilter);

    ui.editSearchDetail->setStyleSheet(ConfigHelper::singleton().getStyleSheetLineEditNormal());

    ui.btnSend->setDisabled(true);

    QObject::connect(ui.listMessage, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(handleListMessageItemDoubleClicked(QListWidgetItem*)));

    QObject::connect(ui.listRecentMessage, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(handleListMessageItemDoubleClicked(QListWidgetItem*)));
    QObject::connect(ui.listRecentMessage, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(handleListMessageCurrentItemChanged(QListWidgetItem*, QListWidgetItem*)));
    QObject::connect(ui.listRecentMessage, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(handleListMessageItemClicked(QListWidgetItem*)));

    QObject::connect(ui.listLogs, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(handleListLogItemCurrentItemChanged(QListWidgetItem*, QListWidgetItem*)));
    QObject::connect(ui.listLogs, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(handleListLogItemClicked(QListWidgetItem*)));
    QObject::connect(ui.listLogs->model(), SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(handleLogInfoAdded(const QModelIndex&, int, int)));

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

    QObject::connect(ui.actionSave, &QAction::triggered, this, &CMainWindow::saveCache);
    QObject::connect(ui.actionLoad, &QAction::triggered, this, &CMainWindow::loadCache);
    QObject::connect(ui.actionClear, &QAction::triggered, this, &CMainWindow::clearCache);
    QObject::connect(ui.actionOptions, &QAction::triggered, this, &CMainWindow::openSettingDlg);
}

CMainWindow::~CMainWindow() {
    delete m_cbKeyPressFilter;
    delete m_listMessageKeyFilter;
    delete m_pPrinter;
}

bool CMainWindow::init() {
    if (ConfigHelper::singleton().isFirstCreateFile()) {
        auto* pDlg = new CSettingDialog(this);
        pDlg->init();
        int ret = pDlg->exec();
        if (ret == QDialog::Rejected) {
            ConfigHelper::singleton().deleteConfigFile();
            exit(0);
        }
    }

    // 初始化日志服务
    m_pPrinter = new CECPrinter(ui.listLogs);
    LogHelper::singleton().setPrinter(m_pPrinter);

    // 初始化通信服务
    NetManager::singleton().init(this);
    LOG_INFO("Initiated network");

    // 注册lua
    LuaScriptSystem::singleton().Setup();
    luaopen_protobuf(LuaScriptSystem::singleton().GetLuaState());
    luaRegisterCppClass();
    LOG_INFO("Initiated lua script system");

    QString luaScriptPath = ConfigHelper::singleton().getLuaScriptPath();
    LOG_INFO("Loading lua script: \"{}\"", luaScriptPath.toStdString());
    if (!LuaScriptSystem::singleton().RunScript(luaScriptPath.toStdString().c_str())) {
        LOG_ERR("Error: Load lua script: \"{}\" failed!", luaScriptPath.toStdString());
        return false;
    }
    LOG_INFO("Lua script: \"{}\" loaded", luaScriptPath.toStdString());

    // 加载proto文件
    LOG_INFO("Importing proto files...");
    if (!importProtos()) {
        return false;
    }

    LuaScriptSystem::singleton().Invoke("_on_proto_reload"
                                        , static_cast<lua_api::IProtoManager*>(&ProtoManager::singleton()));
    LOG_INFO("Import proto files completed");

    // 填充消息列表
    std::list<CProtoManager::MsgInfo> listNames = ProtoManager::singleton().getMsgInfos();
    auto it = listNames.begin();
    for (; it != listNames.end(); ++it) {
        auto* pListItem = new QListWidgetItem();
        pListItem->setText(it->msgName.c_str());
        pListItem->setData(Qt::UserRole, it->msgFullName.c_str());
        ui.listMessage->addItem(pListItem);
    }

    ui.labelMsgCnt->setText(std::to_string(ui.listMessage->count()).c_str());

    // 加载缓存 
    loadCache();

    // 启动主定时器
    auto* pMainTimer = new QTimer(this);
    connect(pMainTimer, &QTimer::timeout, this, QOverload<>::of(&CMainWindow::update));
    pMainTimer->start();

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
        break;
    case kDisconnect:
        ui.btnConnect->setText("Connect");
        ui.btnConnect->setDisabled(false);

        ui.btnSend->setDisabled(true);

        ui.cbIp->setDisabled(false);
        ui.cbAccount->setDisabled(false);
        break;
    case kConnecting:
        ui.btnConnect->setText("Connecting");
        ui.btnConnect->setDisabled(true);

        ui.btnSend->setDisabled(true);

        ui.cbIp->setDisabled(true);
        ui.cbAccount->setDisabled(true);
        break;
    default:
        break;
    }
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
        auto* pMessage = ProtoManager::singleton().createMessage(name);
        if (nullptr == pMessage) {
            LOG_INFO("Cannot find code of message({0})", name);
            return nullptr;
        }

        m_mapMessages[name] = pMessage;
        itr = m_mapMessages.find(name);
    }

    return itr->second;
}

void CMainWindow::saveCache() {
    const std::string& path = ConfigHelper::singleton().getCachePath().toStdString();
    QFile jsonFile(path.c_str());
    jsonFile.open(QIODevice::WriteOnly | QIODevice::Text);
    if (!jsonFile.isOpen()) {
        QMessageBox::warning(this, "", fmt::format("Cannot create: {0}", path).c_str());
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
    std::string path = ConfigHelper::singleton().getCachePath().toStdString();
    if (path.empty()) {
        path = "./cache.json";
        ConfigHelper::singleton().saveCachePath(path.c_str());
    }

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
                        google::protobuf::util::MessageToJsonString(*pMessage, &msgStr, ConfigHelper::singleton().getJsonPrintOption());
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
            pListWidgetItem->setTextColor(Qt::black);
        }
    }
    QMessageBox::information(this, "", fmt::format("Cleared {0} messages.", count).c_str());
}

void CMainWindow::update() {
    NetManager::singleton().run();
}

void CMainWindow::openSettingDlg() {
    auto* pDlg = new CSettingDialog(this);
    pDlg->init();
    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        // TODO (Marskey): reload proto
        QMessageBox::information(this, "", "Modified successfuly, restart the program.");
        exit(0);
    }
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

void CMainWindow::onParseMessage(const char* msgFullName, const char* pData, size_t size) {
    if (ignoreMsgType(msgFullName)) {
        return;
    }

    uint16_t nMessageId = ProtoManager::singleton().getMsgTypeByFullName(msgFullName);
    google::protobuf::Message* pRecvMesage = ProtoManager::singleton().createMessage(msgFullName);
    if (nullptr == pRecvMesage) {
        LOG_INFO("Cannot find code of message({0}:{1})", msgFullName, nMessageId);
        return;
    }

    if (pRecvMesage->ParseFromArray(pData, size)) {
        std::string msgStr;
        google::protobuf::util::MessageToJsonString(*pRecvMesage, &msgStr, ConfigHelper::singleton().getJsonPrintOption());
        addDetailLogInfo(fmt::format("Recieved {0}({1}) size: {2}", msgFullName, nMessageId, pRecvMesage->ByteSize())
                            , highlightJsonData(msgStr.c_str())
                            , QColor(57, 115, 157)
        );

        LuaScriptSystem::singleton().Invoke("_on_message_recv"
                                            , static_cast<lua_api::IClient*>(&m_client)
                                            , msgFullName
                                            , (void*)(&pRecvMesage));
    } else {
        LOG_ERR("Protobuf message {}({}) parse failed!", msgFullName, nMessageId);
    }

    delete pRecvMesage;
}

void CMainWindow::onConnectSucceed(const char* strRemoteIp, Port port, SocketId socketId) {
    LOG_INFO("Connect succeed socket id: {0}, ip: {1}:{2} ", socketId, strRemoteIp, port);
    connectStateChange(kConnected);
    m_client.onConnectSucceed(strRemoteIp, port, socketId);
}

void CMainWindow::onDisconnect(SocketId socketId) {
    LOG_INFO("Connection closed socket id: {}", socketId);
    connectStateChange(kDisconnect);
    m_client.onDisconnect(socketId);
}

void CMainWindow::onError(SocketId socketId, ec_net::ENetError error) {
    switch (error) {
    case ec_net::eNET_CONNECT_FAIL:
        {
            Port port = NetManager::singleton().getRemotePort(socketId);
            std::string strRemoteIp = NetManager::singleton().getRemoteIP(socketId);

            LOG_ERR("Connect {0}:{1} failed", strRemoteIp, port);
            connectStateChange(kDisconnect);
        }
        break;
    case ec_net::eNET_SEND_OVERFLOW:
        LOG_ERR("Send buffer overflow!");
        break;
    case ec_net::eNET_PARSER_NULL:
        LOG_ERR("Parser cannot be null, check if set parser");
        connectStateChange(kDisconnect);
        break;
    default:;
    }

    m_client.onError(socketId, error);
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
    pDlg->restoreGeometry(ConfigHelper::singleton().getSubWindowGeometry());
    pDlg->initDialogByMessage(pMessage);

    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        pMessage->CopyFrom(*pDlg->getMessage());
        std::string msgStr;
        google::protobuf::util::MessageToJsonString(*pMessage, &msgStr, ConfigHelper::singleton().getJsonPrintOption());
        if (!msgStr.empty()) {
            pItem->setIcon(QIcon(":/EmulatorClient/icon1.ico"));
            pItem->setToolTip(msgStr.c_str());

            QList<QListWidgetItem*> listItems = pListWidget->findItems(pItem->text(), Qt::MatchExactly);
            for (int i = 0; i < listItems.count(); ++i) {
                listItems[i]->setToolTip(msgStr.c_str());
            }
        }
    }

    ConfigHelper::singleton().saveSubWindowGeometry(pDlg->saveGeometry());
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

void CMainWindow::handleFilterTextChanged(const QString& text) {
    // 搜索栏搜索逻辑
    for (int i = 0; i < ui.listMessage->count(); ++i) {
        ui.listMessage->setItemHidden(ui.listMessage->item(i), !text.isEmpty());
    }

    bool bIsNumberic = false;
    int msgTypeNum = text.toInt(&bIsNumberic);
    if (bIsNumberic) {
        std::string strMsgName = ProtoManager::singleton().getMsgInfoByMsgType(msgTypeNum).msgName;
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
    unsigned int socketID = m_client.getSocketID();
    if (0 == socketID) {
        QMessageBox::warning(nullptr, "", "Please connect to server first");
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
        m_client.sendMsg(*pMessage);
        std::string msgStr;
        google::protobuf::util::MessageToJsonString(*pMessage, &msgStr, ConfigHelper::singleton().getJsonPrintOption());
        addDetailLogInfo(fmt::format("Sent Message {}", selectMsgName.toStdString()), highlightJsonData(msgStr.c_str()), Qt::black);

        if (0 == ui.listRecentMessage->findItems(selectMsgName, Qt::MatchExactly).count()) {
            auto* pItem = new QListWidgetItem(selectMsgName);
            pItem->setToolTip(msgStr.c_str());
            ui.listRecentMessage->insertItem(0, pItem);
        }

        if (ui.listRecentMessage->count() > 50) {
            delete ui.listRecentMessage->takeItem(ui.listRecentMessage->count());
        }
    }
}

void CMainWindow::handleConnectBtnClicked() {
    unsigned int socketID = m_client.getSocketID();
    if (0 != socketID) {
        LOG_INFO("Disconnecting...");
        NetManager::singleton().disconnect(socketID);
        connectStateChange(kDisconnect);
        return;
    }

    // 判断是不是新的ip和端口，如果是，添加历史
    addNewItemIntoCombox(*ui.cbIp);
    ConfigHelper::singleton().saveWidgetComboxState("Ip_Port", *ui.cbIp);

    QStringList ipAndPort = ui.cbIp->currentText().split(":");
    if (ipAndPort.size() != 2) {
        QMessageBox::warning(this, "", "IP and Port not correct。it should like: 192.168.1.1:28888");
        return;
    }

    if (ui.cbAccount->currentText().isEmpty()) {
        QMessageBox::warning(this, "", "Account cannot be empty");
        return;
    }

    addNewItemIntoCombox(*ui.cbAccount);
    ConfigHelper::singleton().saveWidgetComboxState("Account", *ui.cbAccount);

    std::string ip = ipAndPort[0].toStdString();
    Port port = ipAndPort[1].toUShort();

    LuaScriptSystem::singleton().Invoke("_on_connect_btn_click"
                                        , static_cast<lua_api::IClient*>(&m_client)
                                        , ip
                                        , port
                                        , ui.cbAccount->currentText().toStdString().c_str());

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
    ui.editSearchDetail->setStyleSheet(ConfigHelper::singleton().getStyleSheetLineEditNormal());

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
            ui.editSearchDetail->setStyleSheet(ConfigHelper::singleton().getStyleSheetLineEditError());
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

void CMainWindow::handleLogInfoAdded(const QModelIndex& parent, int start, int end) {
    handleSearchLogTextChanged();

    if (ui.checkIsAutoDetail->isChecked()) {
        ui.listLogs->setCurrentRow(end);
    }
}

void CMainWindow::closeEvent(QCloseEvent* event) {
    ConfigHelper::singleton().saveMainWindowGeometry(saveGeometry());
    ConfigHelper::singleton().saveMainWindowState(saveState());
    ConfigHelper::singleton().saveSplitterH(ui.splitterH->saveState());
    ConfigHelper::singleton().saveSplitterV(ui.splitterV->saveState());
    ConfigHelper::singleton().saveWidgetCheckboxState("AutoShowDetail", *ui.checkIsAutoDetail);

    // 自动保存cache
    saveCache();
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
        if (combox.count() > ConfigHelper::singleton().getHistroyComboxItemMaxCnt()) {
            combox.removeItem(combox.count() - 1);
        }
    } else {
        QString text = combox.currentText();
        combox.removeItem(cbIdx);
        combox.insertItem(0, text);
    }
    combox.setCurrentIndex(0);
}

bool CMainWindow::importProtos() {
    QString strRootPath = ConfigHelper::singleton().getProtoRootPath();
    QString strLoadPath = ConfigHelper::singleton().getProtoFilesLoadPath();

    QDir rootPath = strRootPath;
    ProtoManager::singleton().init(rootPath.absolutePath().toStdString());

    bool bSuccess = false;
    QDirIterator it(strLoadPath, QStringList() << "*.proto", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString virtualPath = rootPath.relativeFilePath(it.next());
        bSuccess |= ProtoManager::singleton().importProto(virtualPath.toStdString());
    }

    if (!bSuccess) {
        LOG_ERR("Import proto failed, check the path is correct.\nProtoRootPath: {0}\nProtoLoadPath: {1}"
                , strRootPath.toStdString()
                , strLoadPath.toStdString());
    }

    return bSuccess;
}

void CMainWindow::luaRegisterCppClass() {
    auto* pLuaState = LuaScriptSystem::singleton().GetLuaState();
    using namespace  lua_api; 
    luabridge::getGlobalNamespace(pLuaState)
        .beginClass<IReadData>("IReadData")
        .addFunction("readUint8", &IReadData::readUint8)
        .addFunction("readInt8", &IReadData::readInt8)
        .addFunction("readUint16", &IReadData::readUint16)
        .addFunction("readInt16", &IReadData::readInt16)
        .addFunction("readUint32", &IReadData::readUint32)
        .addFunction("readInt32", &IReadData::readInt32)
        .addFunction("bindMessage", &IReadData::bindMessage)
        .endClass();

    luabridge::getGlobalNamespace(pLuaState)
        .beginClass<IWriteData>("IWriteData")
        .addFunction("writeUint8", &IWriteData::writeUint8)
        .addFunction("writeUint8", &IWriteData::writeInt8)
        .addFunction("writeUint16", &IWriteData::writeUint16)
        .addFunction("writeInt16", &IWriteData::writeInt16)
        .addFunction("writeUint32", &IWriteData::writeUint32)
        .addFunction("writeInt32", &IWriteData::writeInt32)
        .addFunction("writeBinary", &IWriteData::writeBinary)
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
        .addFunction("sendJsonMsg", &IClient::sendJsonMsg)
        .endClass();
}

void CMainWindow::addDetailLogInfo(std::string msg, std::string detail, QColor color /*= QColor(Qt::GlobalColor(0))*/) {
    time_t t = time(nullptr);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "[%X] ", localtime(&t));

    msg = tmp + msg;
    auto* pListWidgetItem = new QListWidgetItem(msg.c_str());
    pListWidgetItem->setData(Qt::UserRole, detail.c_str());

    if (QColor(Qt::GlobalColor(0)) != color) {
        pListWidgetItem->setForeground(color);
    }

    ui.listLogs->addItem(pListWidgetItem);
}
