#include "ClientMessageSubject.h"
#include "ProtoManager.h"

#include "toolkit/fmt/format.h"
#include "libNetWrapper/NetService.h"

CClientMessageSubject::CClientMessageSubject()
    : m_unSocketID(0) {
}

CClientMessageSubject::~CClientMessageSubject() {
}

void CClientMessageSubject::setMainListener(IClientNetEvent* obj) {
    m_mainListener = obj;
}

void CClientMessageSubject::connectTo(std::string ip, uint16_t port, std::function<void(bool)> callback) {
    m_connectCallback = callback;

    NetService::singleton().connectTo(ip.c_str()
                                      , port
                                      , 1
                                      , 131072
                                      , 131072
                                      , EPackParserType::ePACK_PARSER_NOENCRYPT
                                      , &ClientMessageSubject::singleton());
}

void CClientMessageSubject::doResolveMsg(const IMsg & msg) {
    const char* buffer = msg.getBodyBuf();
    int32_t size = msg.getBodySize();
    uint16_t type = msg.getType();

    // 获取模块
    auto itModule = m_listeners.find(type);
    if (itModule != m_listeners.end()) {
        // 查找注册
        auto itMsg = m_msgs.find(type);
        if (itMsg != m_msgs.end()) {
            // 解析内容
            if (itMsg->second->ParseFromArray(buffer, size)) {
                // 遍历调用
                for (auto& pair : itModule->second) {
                    pair.second();
                }
            }
        }
    }

    m_mainListener->doResolveMsg(msg);
}

bool CClientMessageSubject::doOnConnect(const IMsg & msg, const string & strRemoteIp) {
    m_mainListener->doOnConnect(msg, strRemoteIp);
    return true;
}

void CClientMessageSubject::doOnClose(SocketId unSocketID) {
    m_mainListener->doOnClose(unSocketID);
    m_unSocketID = 0;
}

void CClientMessageSubject::doDoConnect(SocketId unSocketID) {
    m_mainListener->doDoConnect(unSocketID);
    m_unSocketID = unSocketID;
}

bool CClientMessageSubject::doWebConnect(const IMsg& msg) {
    return false;
}

void CClientMessageSubject::onConnectFail(const string & strRemoteIp, uint32_t unPort, int32_t nConnnectType) {
    m_mainListener->onConnectFail(strRemoteIp, unPort, nConnnectType);
    m_connectCallback(false);
}

void CClientMessageSubject::onConnectSuccess(const string & strRemoteIp, uint32_t unPort, int32_t nConnnectType, SocketId unSocketID) {
    m_mainListener->onConnectSuccess(strRemoteIp, unPort, nConnnectType, unSocketID);
    m_connectCallback(true);
}

bool CClientMessageSubject::sendClientMsg(const google::protobuf::Message& message) {
    NetWrapper::MsgHead *pMsg = PackMsg(message);
    if (nullptr == pMsg) {
        //m_pDlg->Log(fmt::format("Cannot Serialize message({0})", pMessage->GetTypeName()));
        return false;
    }

    return NetService::singleton().sendMsg(m_unSocketID
                                           , pMsg
                                           , pMsg->usSize);
}

unsigned int  CClientMessageSubject::getSocketID() {
    return m_unSocketID;
}

NetWrapper::MsgHead * CClientMessageSubject::PackMsg(const google::protobuf::Message & pbMsg) {
    static char MsgBuff[60 * 1024] = {0}; // 发送数据包使用的内存
    NetWrapper::MsgHead *pMsg = reinterpret_cast<NetWrapper::MsgHead*>(MsgBuff);
    if (nullptr == pMsg) {
        return nullptr;
    }

    pMsg->usType = pbMsg.GetType();
    if (0 == pMsg->usType) {
        pMsg->usType = ProtoManager::singleton().getMsgTypeByFullName(pbMsg.GetTypeName());
    }

    pMsg->usSize = MSG_HEAD_SIZE + (uint16_t)pbMsg.ByteSize();
    pMsg->usServerType = 1;
    if (!pbMsg.SerializeToArray(MsgBuff + MSG_HEAD_SIZE
                                , pMsg->usSize - MSG_HEAD_SIZE)) {
        return nullptr;
    }

    return pMsg;
}
