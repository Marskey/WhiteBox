#include "EmulatorClient.h"
#include "ProtoManager.h"
#include "MsgEditorDialog.h"
#include "ClientMessageSubject.h"
#include "Style.h"

#include <QTime>
#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>

#include <fstream>

#include "toolkit/fmt/format.h"
#include "libNetWrapper/NetService.h"
#include "MsgIgnoreDialog.h"
#include "ProtoSelectDialog.h"

#define MAX_LIST_HISTROY_CNT 10
const char WINDOWS_TITLE[25] = "EmuCl";

CEmulatorClient::CEmulatorClient(QWidget* parent)
    : QMainWindow(parent)
{
    const CEmulatorClient::ECVersion ecVer = { 2
                                              , 1
                                              , 7 };
    ui.setupUi(this);

    g_option.add_whitespace = true;
    g_option.always_print_primitive_fields = true;
    g_option.preserve_proto_field_names = true;
    setWindowTitle(WINDOWS_TITLE);

    m_configIni = new QSettings("config.ini", QSettings::IniFormat);

    QString strRootPath = m_configIni->value("/path/proto_root_path").toString();
    QString strLoadPath = m_configIni->value("/path/proto_path").toString();

    if (strLoadPath.isEmpty()
        || strRootPath.isEmpty()) {
        CProtoSelectDialog* pDlg = new CProtoSelectDialog(this);
        pDlg->init(strRootPath, strLoadPath);
        int ret = pDlg->exec();
        if (ret == QDialog::Accepted) {
            strRootPath = pDlg->getRootPath();
            strLoadPath = pDlg->getLoadPath();
        }
        else if (ret == QDialog::Rejected) {
            exit(0);
        }
    }

    QDir protoRootPath = strRootPath;
    QDir protoPath = strLoadPath;

    std::string strProtoRootPath = protoRootPath.path().toStdString();
    if (protoRootPath.isRelative()) {
        strProtoRootPath = protoRootPath.absolutePath().toStdString();
    }

    std::string strProtoPath = protoPath.path().toStdString();
    if (protoPath.isRelative()) {
        strProtoPath = protoPath.absolutePath().toStdString();
    }

    if (!ProtoManager::singleton().importMsg(strProtoPath.c_str(), strProtoRootPath.c_str())) {
        ErrLog(fmt::format("Import proto failed, check the path is correct or not.\nproto_root_path: {0}\nproto_path: {1}", strProtoRootPath, strProtoPath));
        return;
    }

    // 导入成功保存
    m_configIni->setValue("/path/proto_root_path", strProtoRootPath.c_str());
    m_configIni->setValue("/path/proto_path", strProtoPath.c_str());

    log("Import proto completed");

    // 读取用户名
    ui.cbAccount->setLineEdit(new CLineEdit(this));
    QStringList accountList = m_configIni->value("/default/account").toString().split(",");
    for (int i = 0; i < accountList.count(); ++i) {
        if (accountList[i].isEmpty()) {
            continue;
        }
        ui.cbAccount->addItem(accountList[i]);
    }
    ui.cbAccount->setCurrentIndex(0);
    ui.cbAccount->view()->installEventFilter(m_cbKeyPressFilter);

    // 读取角色id
    ui.cbPlayerId->setLineEdit(new CLineEdit(this));
    QStringList playerIdList = m_configIni->value("/default/playerId").toString().split(",");
    for (int i = 0; i < playerIdList.count(); ++i) {
        if (playerIdList[i].isEmpty()) {
            continue;
        }
        ui.cbPlayerId->addItem(playerIdList[i]);
    }
    ui.cbPlayerId->setCurrentIndex(0);
    ui.cbPlayerId->lineEdit()->setValidator(new QDoubleValidator(0, INT64_MAX, 0, ui.cbPlayerId));
    ui.cbPlayerId->view()->installEventFilter(m_cbKeyPressFilter);

    // 读取kingdomId
    QString defaultKingdomId = m_configIni->value("/default/kingdomId", "0").toString();
    ui.editKingdomId->setValidator(new QIntValidator(0, INT32_MAX, ui.editKingdomId));
    ui.editKingdomId->setText(defaultKingdomId);

    // 读取ip
    ui.cbIp->setLineEdit(new CLineEdit(this));
    QStringList ipList = m_configIni->value("/default/ip").toString().split(",");
    for (int i = 0; i < ipList.count(); ++i) {
        if (ipList[i].isEmpty()) {
            continue;
        }
        ui.cbIp->addItem(ipList[i]);
    }
    ui.cbIp->setCurrentIndex(0);
    ui.cbIp->view()->installEventFilter(m_cbKeyPressFilter);

    // 读取端口
    std::string defaultPort = m_configIni->value("/default/port").toString().toStdString();
    ui.editPort->setText(defaultPort.c_str());

    // 读取是否使用uuid登入
    ui.useDevMode->setCheckState(static_cast<Qt::CheckState>(m_configIni->value("/default/use_uuid").toInt()));

    // 读取是否自动显示最新
    ui.checkIsAutoDetail->setCheckState(static_cast<Qt::CheckState>(m_configIni->value("/default/auto_show_detail").toInt()));

    // 读取上次关闭时的窗口大小
    QSize winSize = m_configIni->value("/default/win_size").toSize();
    if (winSize.isValid()) {
        resize(winSize);
    }

    m_subWindowSize = m_configIni->value("/default/sub_win_size").toSize();

    // 恢复上次的布局大小
    ui.splitterV->restoreState(m_configIni->value("/default/layout_split_v").toByteArray());
    ui.splitterH->restoreState(m_configIni->value("/default/layout_split_h").toByteArray());

    // 填充消息列表
    std::list<CProtoManager::MsgInfo> listNames = ProtoManager::singleton().getMsgInfos();
    auto it = listNames.begin();
    for (; it != listNames.end(); ++it) {
        QListWidgetItem* pListItem = new QListWidgetItem();
        pListItem->setText(it->msgName.c_str());
        pListItem->setWhatsThis(it->msgFullName.c_str());
        ui.listMessage->addItem(pListItem);
    }

    ui.listMessage->installEventFilter(m_listMessageKeyFilter);
    ui.listRecentMessage->installEventFilter(m_listMessageKeyFilter);

    ui.labelMsgCnt->setText(std::to_string(ui.listMessage->count()).c_str());

    // 初始化通信服务
    NetService::singleton().init(&ClientMessageSubject::singleton());

    // 设置监听对象
    ClientMessageSubject::singleton().setMainListener(this);

    ui.btnSend->setDisabled(true);

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&CEmulatorClient::update));
    timer->start(40);

    initQtObjectConnect();

    ui.editSearchDetail->setStyleSheet(styleSheetLineEditNormal);

    // 加载缓存 
    load();
    ui.lableIgnoreCnt->setText(std::to_string(m_setIgnoredReceiveMsgType.size()).c_str());

    log(fmt::format("Ready.             - Version {}.{}.{} By marskey.", ecVer.main, ecVer.sub, ecVer.build));
}

void CEmulatorClient::doResolveMsg(const IMsg& msg) {
    uint16_t uMessageType = msg.getType();

    if (ignoreMsgType(msg.getType())) {
        return;
    }

    std::string msgFullName = ProtoManager::singleton().getMsgInfoByMsgType(uMessageType).msgFullName;

    const google::protobuf::Descriptor* pDescriptor = ProtoManager::singleton().findMessageByName(msgFullName);

    if (pDescriptor == NULL) {
        log(fmt::format("Cannot find code of message({0}:{1})", msgFullName, uMessageType));
        return;
    }

    google::protobuf::Message* pRecvMesage = ProtoManager::singleton().createMessage(pDescriptor);
    if (pRecvMesage->ParseFromArray(msg.getBodyBuf(), msg.getBodySize())) {
        std::string msgStr;
        google::protobuf::util::MessageToJsonString(*pRecvMesage, &msgStr, g_option);
        log(fmt::format("Recieved {0}({1}) size: {2}", msgFullName, uMessageType, pRecvMesage->ByteSize())
            , QColor(57, 115, 157)
            , highlightJsonData(msgStr.c_str()));
    }
    delete pRecvMesage;

    if (m_loginModule.isReady()) {
        connectStateChange(kConnected);
    }
}

void CEmulatorClient::doOnClose(SocketId unSocketID) {
    log(fmt::format("Connection closed socket id: {}", unSocketID));
    connectStateChange(kDisconnect);
}

void CEmulatorClient::doDoConnect(SocketId unSocketID) {
}

void CEmulatorClient::onConnectFail(const std::string& strRemoteIp, uint32_t unPort, int32_t nConnnectType) {
    ErrLog(fmt::format("Connect {0}:{1} failed", strRemoteIp, unPort));
    connectStateChange(kDisconnect);
}

void CEmulatorClient::onConnectSuccess(const std::string& strRemoteIp, uint32_t unPort, int32_t nConnnectType, SocketId unSocketID) {
    log(fmt::format("Connect succeed socket id: {0}, ip: {1}:{2} ", unSocketID, strRemoteIp, unPort));
}

void CEmulatorClient::initQtObjectConnect() {
    QObject::connect(ui.listMessage, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(handleListMessageItemDoubleClicked(QListWidgetItem*)));
    //QObject::connect(ui.listMessage, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(handleListMessageCurrentItemChanged(QListWidgetItem*, QListWidgetItem*)));
    //QObject::connect(ui.listMessage, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(handleListMessageItemClicked(QListWidgetItem*)));

    QObject::connect(ui.listRecentMessage, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(handleListMessageItemDoubleClicked(QListWidgetItem*)));
    QObject::connect(ui.listRecentMessage, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(handleListMessageCurrentItemChanged(QListWidgetItem*, QListWidgetItem*)));
    QObject::connect(ui.listRecentMessage, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(handleListMessageItemClicked(QListWidgetItem*)));

    QObject::connect(ui.listLogs, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(handleListLogItemCurrentItemChanged(QListWidgetItem*, QListWidgetItem*)));
    QObject::connect(ui.listLogs, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(handleListLogItemCliecked(QListWidgetItem*)));

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

    QObject::connect(ui.checkIsAutoDetail, SIGNAL(stateChanged(int)), this, SLOT(handledStateChanged(int)));

    QObject::connect(ui.actionSave, &QAction::triggered, this, &CEmulatorClient::save);
    QObject::connect(ui.actionLoad, &QAction::triggered, this, &CEmulatorClient::load);
    QObject::connect(ui.actionClear, &QAction::triggered, this, &CEmulatorClient::clear);
    QObject::connect(ui.actionSelectProtoPath, &QAction::triggered, this, &CEmulatorClient::reSelectProtoPath);
}

void CEmulatorClient::connectStateChange(EConnectState state) {
    switch (state) {
    case kConnected:
        ui.btnConnect->setText("Disconnect");
        ui.btnConnect->setDisabled(false);

        ui.btnSend->setDisabled(false);

        ui.cbIp->setDisabled(true);
        ui.editPort->setDisabled(true);
        ui.cbAccount->setDisabled(true);
        ui.useDevMode->setDisabled(true);
        break;
    case kDisconnect:
        ui.btnConnect->setText("Connect");
        ui.btnConnect->setDisabled(false);

        ui.btnSend->setDisabled(true);

        ui.cbIp->setDisabled(false);
        ui.editPort->setDisabled(false);
        ui.cbAccount->setDisabled(false);
        ui.useDevMode->setDisabled(false);
        break;
    case kConnecting:
        ui.btnConnect->setText("Connecting");
        ui.btnConnect->setDisabled(true);

        ui.btnSend->setDisabled(true);

        ui.cbIp->setDisabled(true);
        ui.editPort->setDisabled(true);
        ui.cbAccount->setDisabled(true);
        ui.useDevMode->setDisabled(true);
        break;
    default:
        break;
    }
}

bool CEmulatorClient::ignoreMsgType(uint16_t uMsgType) {
    if (m_setIgnoredReceiveMsgType.find(uMsgType) != m_setIgnoredReceiveMsgType.end()) {
        return true;
    }

    return false;
}

google::protobuf::Message* CEmulatorClient::getMessageByName(const char* name) {
    auto itr = m_mapMessages.find(name);
    if (itr == m_mapMessages.end()) {
        return nullptr;
    }

    return itr->second;
}

google::protobuf::Message* CEmulatorClient::getOrCreateMessageByName(const char* name) {
    auto itr = m_mapMessages.find(name);
    if (itr == m_mapMessages.end()) {
        const google::protobuf::Descriptor* pDescriptor = ProtoManager::singleton().findMessageByName(name);

        if (pDescriptor == NULL) {
            log(fmt::format("Cannot find code of message({0})", name));
            return nullptr;
        }

        m_mapMessages[name] = ProtoManager::singleton().createMessage(pDescriptor);
        itr = m_mapMessages.find(name);
    }

    return itr->second;
}

void CEmulatorClient::log(std::string msg, QColor color /*= QColor(Qt::GlobalColor(0))*/, std::string detail /*= ""*/) {
    time_t t = time(0);
    char tmp[64];
    //strftime(tmp, sizeof(tmp), "[%Y-%m-%d %X] ", localtime(&t));
    strftime(tmp, sizeof(tmp), "[%X] ", localtime(&t));

    msg = tmp + msg;
    QListWidgetItem* pListWidgetItem = new QListWidgetItem(msg.c_str());
    pListWidgetItem->setWhatsThis(detail.c_str());

    if (QColor(Qt::GlobalColor(0)) != color) {
        pListWidgetItem->setTextColor(color);
    }

    ui.listLogs->addItem(pListWidgetItem);

    handleSearchLogTextChanged();

    if (ui.checkIsAutoDetail->isChecked()) {
        ui.listLogs->setCurrentItem(pListWidgetItem);
    }
}

void CEmulatorClient::ErrLog(std::string msg) {
    log("Error: " + msg, Qt::red);
}

void CEmulatorClient::WarnLog(std::string msg) {
    log("Warnning: " + msg, Qt::darkYellow);
}

void CEmulatorClient::save() {
    std::string path = "data.bytes";
    std::ifstream f(path, std::ios::binary);

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        QMessageBox::warning(this, "", fmt::format("Cannot create: {0}", path).c_str());
        return;
    }

    size_t count = m_mapMessages.size();
    file.write((char*)&count, sizeof(size_t));
    for (const auto& pair : m_mapMessages) {
        size_t fileNameCnt = pair.first.length();
        file.write((char*)&fileNameCnt, sizeof(size_t));
        file.write(pair.first.c_str(), fileNameCnt);
        char data[64 * 1024] = { 0 };
        pair.second->SerializeToArray(data, 64 * 1024);
        int dataLen = pair.second->ByteSize();
        file.write((char*)&dataLen, sizeof(int));
        file.write(data, dataLen);
    }

    count = m_setIgnoredReceiveMsgType.size();
    file.write((char*)&count, sizeof(size_t));
    for (const auto& it : m_setIgnoredReceiveMsgType) {
        file.write((char*)(&it), sizeof(uint16_t));
    }
}

void CEmulatorClient::load() {
    std::string path = "data.bytes";
    std::ifstream f(path, std::ios::binary);
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        WarnLog(fmt::format("Warning: no cache file:{} were found.", path).c_str());
        return;
    }

    bool bSuccess = true;

    size_t dataCnt = 0;
    file.read((char*)&dataCnt, sizeof(size_t));

    for (int i = 0; i < dataCnt; ++i) {
        size_t fileNameLen = 0;
        file.read((char*)&fileNameLen, sizeof(size_t));
        char messageName[64] = { 0 };
        file.read(messageName, fileNameLen);

        int dataLen = 0;
        file.read((char*)&dataLen, sizeof(int));
        char data[64 * 1024] = { 0 };
        file.read(data, dataLen);

        google::protobuf::Message* pMessage = getOrCreateMessageByName(messageName);
        if (nullptr == pMessage) {
            QMessageBox::critical(this, "Error", fmt::format("Cache file {} is corrupted!", path).c_str());
            bSuccess = false;
            break;
        }

        if (pMessage->ParseFromArray(data, dataLen)) {
            m_mapMessages[messageName] = pMessage;

            std::string msgStr;
            for (int i = 0; i < ui.listMessage->count(); ++i) {
                QListWidgetItem* pItem = ui.listMessage->item(i);
                if (nullptr != pItem) {
                    std::string fullName = pItem->whatsThis().toStdString();
                    if (fullName == messageName) {
                        pItem->setIcon(QIcon(":/EmulatorClient/icon1.ico"));
                        google::protobuf::util::MessageToJsonString(*pMessage, &msgStr, g_option);
                        pItem->setToolTip(msgStr.c_str());
                    }
                }
            }
        }
    }

    size_t ignoreMsgCnt = 0;
    file.read((char*)&ignoreMsgCnt, sizeof(size_t));
    uint16_t msgType = 0;
    for (int i = 0; i < ignoreMsgCnt; ++i) {
        file.read((char*)&msgType, sizeof(uint16_t));
        m_setIgnoredReceiveMsgType.insert(msgType);
    }

    ui.labelCacheCnt->setText(std::to_string(m_mapMessages.size()).c_str());

    if (bSuccess) {
        log(fmt::format("Cache loaded successfuly: {0} messages, {1} Ignore message type.", dataCnt, ignoreMsgCnt).c_str(), Qt::darkGreen);
    }
    else {
        ErrLog(fmt::format("Cache file {} is corrupted! please delete it or re-save it.", path));
    }
}

void CEmulatorClient::clear() {
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

void CEmulatorClient::update() {
    NetService::singleton().run();
}

void CEmulatorClient::reSelectProtoPath() {
    QString strRootPath = m_configIni->value("/path/proto_root_path").toString();
    QString strLoadPath = m_configIni->value("/path/proto_path").toString();

    CProtoSelectDialog* pDlg = new CProtoSelectDialog(this);
    pDlg->init(strRootPath, strLoadPath);
    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        bool bNeedRestart = false;
        if (pDlg->getRootPath() != strRootPath) {
            strRootPath = pDlg->getRootPath();
            m_configIni->setValue("/path/proto_root_path", strRootPath);
            bNeedRestart = true;
        }

        if (pDlg->getLoadPath() != strLoadPath) {
            strLoadPath = pDlg->getLoadPath();
            m_configIni->setValue("/path/proto_path", strLoadPath);
            bNeedRestart = true;
        }

        if (bNeedRestart) {
            QMessageBox::information(this, "", "Modified successfuly, restart the program.");
            exit(0);
        }
    }
}

std::string CEmulatorClient::highlightJsonData(const QString& jsonData) {
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
            }
            else {
                style = "color: #256d05;"; // string
            }
        }
        else if (subMatch2.hasMatch()) {
            style = "color: #7ebdff"; // bool
        }

        QString newWord = "<span style=\"" + style + "\">" + word + "</span>";
        result.replace(match.capturedStart() + offset, match.capturedLength(), newWord);
        offset += newWord.length() - match.capturedLength();
    }

    return result.toStdString();
}

void CEmulatorClient::handleListMessageItemDoubleClicked(QListWidgetItem* pItem) {
    if (nullptr == pItem) {
        return;
    }

    if (pItem->text().isEmpty()) {
        return;
    }

    std::string msgFullName = pItem->whatsThis().toStdString();

    google::protobuf::Message* pMessage = getOrCreateMessageByName(msgFullName.c_str());
    if (nullptr == pMessage) {
        return;
    }

    QListWidget* pListWidget = ui.listMessage;
    if (pItem->listWidget() == ui.listMessage) {
        pListWidget = ui.listRecentMessage;
    }

    CMsgEditorDialog* pDlg = new CMsgEditorDialog(this);
    if (m_subWindowSize.isValid()) {
        pDlg->resize(m_subWindowSize);
    }

    pDlg->initDialogByMessage(pMessage);
    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        pMessage->CopyFrom(*pDlg->getMessage());
        std::string msgStr;
        google::protobuf::util::MessageToJsonString(*pMessage, &msgStr, g_option);
        if (!msgStr.empty()) {
            pItem->setIcon(QIcon(":/EmulatorClient/icon1.ico"));
            pItem->setToolTip(msgStr.c_str());

            QList<QListWidgetItem*> listItems = pListWidget->findItems(pItem->text(), Qt::MatchExactly);
            for (int i = 0; i < listItems.count(); ++i) {
                listItems[i]->setToolTip(msgStr.c_str());
            }
        }
    }

    m_subWindowSize = pDlg->size();
    delete pDlg;

    if (pMessage->ByteSize() == 0) {
        delete pMessage;
        m_mapMessages.erase(msgFullName.c_str());

        pItem->setIcon(QIcon());
        pItem->setToolTip("");

        QList<QListWidgetItem*> listItems = pListWidget->findItems(pItem->text(), Qt::MatchExactly);
        for (int i = 0; i < listItems.count(); ++i) {
            listItems[i]->setToolTip("");
        }
    }

    ui.labelCacheCnt->setText(std::to_string(m_mapMessages.size()).c_str());
}

void CEmulatorClient::handleListMessageCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous) {
    ui.textEdit->clear();

    QListWidgetItem* pItem = current;
    if (nullptr == pItem) {
        return;
    }

    if (pItem->text().isEmpty()) {
        return;
    }

    std::string msgFullName = pItem->whatsThis().toStdString();

    google::protobuf::Message* pMessage = getMessageByName(msgFullName.c_str());
    if (nullptr == pMessage) {
        return;
    }

    ui.textEdit->setHtml(fmt::format("<!DOCTYPE html><html><body>{}<hr><pre>{}</pre></body></html>", pItem->text().toStdString(), highlightJsonData(pItem->toolTip())).c_str());
    handleSearchDetailTextChanged();
}

void CEmulatorClient::handleListMessageItemClicked(QListWidgetItem* pItem) {
    handleListMessageCurrentItemChanged(pItem, nullptr);
}

void CEmulatorClient::handleListLogItemCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous) {
    QListWidgetItem* pItem = current;
    if (nullptr == pItem) {
        return;
    }

    if (pItem->text().isEmpty()) {
        return;
    }

    ui.textEdit->clear();
    ui.textEdit->setHtml(fmt::format("<!DOCTYPE html><html><body>{}<hr><pre>{}</pre></body></html>", pItem->text().toStdString(), pItem->whatsThis().toStdString()).c_str());
    handleSearchDetailTextChanged();
}

void CEmulatorClient::handleListLogItemCliecked(QListWidgetItem* pItem) {
    handleListLogItemCurrentItemChanged(pItem, nullptr);
}

void CEmulatorClient::handleFilterTextChanged(const QString& text) {
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
    }
    else {
        QString rexPattern = text;
        rexPattern = rexPattern.replace(" ", ".*");
        rexPattern.prepend(".*");
        rexPattern.append(".*");
        QList<QListWidgetItem*> listFound = ui.listMessage->findItems(rexPattern, Qt::MatchRegExp);
        for (int i = 0; i < listFound.count(); ++i) {
            ui.listMessage->setItemHidden(listFound[i], false);
        }
    }
}

void CEmulatorClient::handleSendBtnClicked() {
    unsigned int unSocketID = ClientMessageSubject::singleton().getSocketID();
    if (0 == unSocketID) {
        QMessageBox msgBox;
        msgBox.setText("Please connect to server first.");
        msgBox.exec();
        return;
    }

    int idx = ui.tabWidget->currentIndex();
    QListWidgetItem* pSelectItem = nullptr;
    if (0 == idx) {
        pSelectItem = ui.listMessage->currentItem();
    }
    else if (1 == idx) {
        pSelectItem = ui.listRecentMessage->currentItem();
    }

    if (nullptr == pSelectItem) {
        QMessageBox msgBox;
        msgBox.setText("Please select one to send.");
        msgBox.exec();
        return;
    }

    QString selectMsgName = pSelectItem->text();
    std::string msgFullName = pSelectItem->whatsThis().toStdString();

    google::protobuf::Message* pMessage = getOrCreateMessageByName(msgFullName.c_str());
    if (nullptr != pMessage) {
        ClientMessageSubject::singleton().sendClientMsg(*pMessage);
        std::string msgStr;
        google::protobuf::util::MessageToJsonString(*pMessage, &msgStr, g_option);
        log(fmt::format("Sent Message {}", selectMsgName.toStdString()), Qt::black, highlightJsonData(msgStr.c_str()));

        if (0 == ui.listRecentMessage->findItems(selectMsgName, Qt::MatchExactly).count()) {
            QListWidgetItem* pItem = new QListWidgetItem(selectMsgName);
            pItem->setToolTip(msgStr.c_str());
            ui.listRecentMessage->insertItem(0, pItem);
        }

        if (ui.listRecentMessage->count() > 50) {
            delete ui.listRecentMessage->takeItem(ui.listRecentMessage->count());
        }
    }
}

void CEmulatorClient::handleConnectBtnClicked() {
    unsigned int unSocketID = ClientMessageSubject::singleton().getSocketID();
    if (0 == unSocketID) {
        int cbIpIdx = ui.cbIp->findText(ui.cbIp->currentText());
        if (-1 == cbIpIdx) {
            ui.cbIp->insertItem(0, ui.cbIp->currentText());
            if (ui.cbIp->count() > MAX_LIST_HISTROY_CNT) {
                ui.cbIp->removeItem(ui.cbIp->count() - 1);
            }
        }
        else {
            QString text = ui.cbIp->currentText();
            ui.cbIp->removeItem(cbIpIdx);
            ui.cbIp->insertItem(0, text);
        }
        ui.cbIp->setCurrentIndex(0);

        QStringList ipList;
        for (int index = 0; index < ui.cbIp->count(); index++) {
            ipList.append(ui.cbIp->itemText(index));
        }
        m_configIni->setValue("/default/ip", ipList.join(","));
        m_configIni->setValue("/default/port", ui.editPort->text());
        m_configIni->setValue("/default/use_uuid", ui.useDevMode->checkState());

        if (ui.cbAccount->currentText().isEmpty()) {
            QMessageBox msgBox;
            msgBox.setText("Account cannot be EMPTY!");
            msgBox.exec();
            return;
        }

        int cbAccountIdx = ui.cbAccount->findText(ui.cbAccount->currentText());
        if (-1 == cbAccountIdx) {
            ui.cbAccount->insertItem(0, ui.cbAccount->currentText());
            if (ui.cbAccount->count() > MAX_LIST_HISTROY_CNT) {
                ui.cbAccount->removeItem(ui.cbAccount->count() - 1);
            }
        }
        else {
            QString text = ui.cbAccount->currentText();
            ui.cbAccount->removeItem(cbAccountIdx);
            ui.cbAccount->insertItem(0, text);
        }
        ui.cbAccount->setCurrentIndex(0);

        QStringList accountList;
        for (int index = 0; index < ui.cbAccount->count(); index++) {
            accountList.append(ui.cbAccount->itemText(index));
        }
        m_configIni->setValue("/default/account", accountList.join(","));

        if (!ui.cbPlayerId->currentText().isEmpty()) {
            int cbPlayerIdIdx = ui.cbPlayerId->findText(ui.cbPlayerId->currentText());
            if (-1 == cbPlayerIdIdx) {
                ui.cbPlayerId->insertItem(0, ui.cbPlayerId->currentText());
                if (ui.cbPlayerId->count() > MAX_LIST_HISTROY_CNT) {
                    ui.cbPlayerId->removeItem(ui.cbPlayerId->count() - 1);
                }
            }
            else {
                QString text = ui.cbPlayerId->currentText();
                ui.cbPlayerId->removeItem(cbPlayerIdIdx);
                ui.cbPlayerId->insertItem(0, text);
            }
            ui.cbPlayerId->setCurrentIndex(0);

            QStringList playerIdList;
            for (int index = 0; index < ui.cbPlayerId->count(); index++) {
                playerIdList.append(ui.cbPlayerId->itemText(index));
            }
            m_configIni->setValue("/default/playerId", playerIdList.join(","));
        }

        m_configIni->setValue("/default/kingdomId", ui.editKingdomId->text());

        // 登入模块
        m_loginModule.connectLS(ui.cbIp->currentText().toStdString()
                                , ui.editPort->text().toInt()
                                , ui.cbAccount->currentText().toStdString()
                                , ui.useDevMode->isChecked()
                                , ui.editKingdomId->text().toInt()
                                , ui.cbPlayerId->currentText().toInt());

        connectStateChange(kConnecting);
        log(fmt::format("Connecting to {}:{}...", ui.cbIp->currentText().toStdString(), ui.editPort->text().toStdString()));

        setWindowTitle(fmt::format("{}:{}", WINDOWS_TITLE, ui.cbAccount->currentText().toStdString()).c_str());
    }
    else {
        NetService::singleton().disConnect(unSocketID);
        log("Disconnecting...");
        connectStateChange(kDisconnect);
    }
}

void CEmulatorClient::handleSearchDetailTextChanged() {
    // 详情搜索栏搜索逻辑
    QString searchString = ui.editSearchDetail->text();
    QTextDocument* document = ui.textEdit->document();

    document->undo();
    document->undo();

    m_vecSearchPos.clear();
    m_searchResultIdx = -1;
    ui.editSearchDetail->setStyleSheet(styleSheetLineEditNormal);

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
        }
        else {
            ui.editSearchDetail->setStyleSheet(styleSheetLineEditError);
        }
    }

    ui.labelSearchCnt->setText(fmt::format("{}/{}", m_searchResultIdx + 1, m_vecSearchPos.size()).c_str());
}

void CEmulatorClient::handleSearchDetailBtnLastResult() {
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

void CEmulatorClient::handleSearchDetailBtnNextResult() {
    if (-1 == m_searchResultIdx) {
        return;
    }

    if (m_searchResultIdx + 1 >= m_vecSearchPos.size()) {
        m_searchResultIdx = 0;
    }
    else {
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

void CEmulatorClient::handleSearchLogTextChanged() {
    // 日志搜索栏搜索逻辑
    m_listFoundItems.clear();
    m_searchLogResultIdx = -1;
    QString searchString = ui.editLogSearch->text();
    if (searchString.isEmpty()) {
        for (int i = 0; i < ui.listLogs->count(); ++i) {
            QListWidgetItem* pItem = ui.listLogs->item(i);
            pItem->setBackground(Qt::white);
        }
    }
    else {
        m_listFoundItems = ui.listLogs->findItems(searchString, Qt::MatchContains);
        for (int i = 0; i < ui.listLogs->count(); ++i) {
            QListWidgetItem* pItem = ui.listLogs->item(i);
            if (m_listFoundItems.contains(pItem)) {
                pItem->setBackground(Qt::yellow);
            }
            else {
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
            m_listFoundItems[m_searchLogResultIdx]->setBackground(QColor(255, 150, 50));
        }
        else {
            m_searchLogResultIdx = -1;
        }
    }

    ui.labelLogSearchCnt->setText(fmt::format("{}/{}", m_searchLogResultIdx + 1, m_listFoundItems.size()).c_str());
}

void CEmulatorClient::handleSearchLogBtnLastResult() {
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
    ui.labelLogSearchCnt->setText(fmt::format("{}/{}", m_searchLogResultIdx + 1, m_listFoundItems.size()).c_str());
}

void CEmulatorClient::handleSearchLogBtnNextResult() {
    if (-1 == m_searchLogResultIdx) {
        return;
    }

    m_listFoundItems[m_searchLogResultIdx]->setBackground(Qt::yellow);

    if (m_searchLogResultIdx + 1 >= m_listFoundItems.size()) {
        m_searchLogResultIdx = 0;
    }
    else {
        m_searchLogResultIdx++;
    }

    m_listFoundItems[m_searchLogResultIdx]->setBackground(QColor(255, 150, 50));
    ui.listLogs->scrollToItem(m_listFoundItems[m_searchLogResultIdx]);
    ui.labelLogSearchCnt->setText(fmt::format("{}/{}", m_searchLogResultIdx + 1, m_listFoundItems.size()).c_str());
}

void CEmulatorClient::handleBtnIgnoreMsgClicked() {
    CMsgIgnoreDialog* pDlg = new CMsgIgnoreDialog(this);
    pDlg->init(m_setIgnoredReceiveMsgType);
    int ret = pDlg->exec();
    if (ret == QDialog::Accepted) {
        m_setIgnoredReceiveMsgType.clear();
        const std::list<std::string>& listIgnoredMsg = pDlg->getIgnoredMsg();
        auto& it = listIgnoredMsg.begin();
        for (; it != listIgnoredMsg.end(); ++it) {
            uint16_t msgType = ProtoManager::singleton().getMsgTypeByFullName(*it);
            m_setIgnoredReceiveMsgType.insert(msgType);
        }

        ui.lableIgnoreCnt->setText(std::to_string(m_setIgnoredReceiveMsgType.size()).c_str());
    }

    delete pDlg;
}

void CEmulatorClient::handledStateChanged(int state) {
    m_configIni->setValue("/default/auto_show_detail", state);
}

void CEmulatorClient::closeEvent(QCloseEvent* event) {
    m_configIni->setValue("/default/win_size", size());
    m_configIni->setValue("/default/sub_win_size", m_subWindowSize);

    m_configIni->setValue("/default/layout_split_v", ui.splitterV->saveState());
    m_configIni->setValue("/default/layout_split_h", ui.splitterH->saveState());

    // 自动保存cache
    save();
}

void CEmulatorClient::keyPressEvent(QKeyEvent* event) {
    // 上一个搜索结果
    if (event->modifiers() == Qt::ShiftModifier && event->key() == Qt::Key::Key_F3) {
        if (ui.editSearchDetail->text().isEmpty()) {
            return;
        }

        handleSearchDetailBtnLastResult();
    }
    else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key::Key_F3) {
        // 搜索
        QTextCursor cursor = ui.textEdit->textCursor();
        if (cursor.hasSelection()) {
            ui.editSearchDetail->setText(cursor.selectedText());
        }
    }
    else if (event->key() == Qt::Key::Key_F3) {
        // 下一个搜索结果
        if (ui.editSearchDetail->text().isEmpty()) {
            return;
        }

        handleSearchDetailBtnNextResult();
    }
}
