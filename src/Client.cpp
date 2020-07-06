#include "Client.h"
#include "NetManager.h"
#include "ProtoManager.h"
#include "LuaScriptSystem.h"

#include <google/protobuf/util/json_util.h>

CClient::CClient() {
    m_sendMsgDataBuf.reserve(MAX_BUFF_SIZE);
}

void CClient::connectTo(std::string ip, Port port, std::function<void(bool)> callback) {
    m_connectCallback = callback;

    NetManager::singleton().connect(ip.c_str(), port, 131073,131073);
}

bool CClient::sendMsg(const google::protobuf::Message& message) {
    size_t messageSize = message.ByteSizeLong();
    if (messageSize > m_sendMsgDataBuf.capacity()) {
        m_sendMsgDataBuf.reserve(messageSize);
    }

    if (!message.SerializeToArray(m_sendMsgDataBuf.data(), m_sendMsgDataBuf.capacity())) {
        return false;
    }

    return NetManager::singleton().sendProtoMsg(m_socketID
                                                , message.GetTypeName().c_str()
                                                , m_sendMsgDataBuf.data()
                                                , message.ByteSizeLong());
}

unsigned int CClient::getSocketID() {
    return m_socketID;
}

void CClient::disconnect() {
    if (m_socketID != 0) {
        NetManager::singleton().disconnect(m_socketID);
        m_socketID = 0;
    }
}

void CClient::onConnectSucceed(const char* strRemoteIp, Port port, SocketId socketId) {
    m_socketID = socketId;
    m_connectCallback(true);
}

void CClient::onDisconnect(SocketId socketId) {
    m_socketID = 0;
}

void CClient::onError(SocketId socketId, ec_net::ENetError error) {
    m_connectCallback(false);
}

void CClient::onParseMessage(const char* msgFullName, const char* pData, size_t size) {
}

void CClient::connect(const char* ip, Port port, BuffSize recv, BuffSize send, const char* tag) {
    NetManager::singleton().connect(ip, port, recv, send);
    m_connectCallback = [this, t = std::string(tag)] (bool bResult) {
        if (!bResult) {
            return;
        }

        LuaScriptSystem::singleton().Invoke("_on_client_connected"
                                            , static_cast<lua_api::IClient*>(this)
                                            , t);
    };
}

void CClient::sendJsonMsg(const char* msgFullName, const char* jsonData) {
    google::protobuf::Message* pMessage = ProtoManager::singleton().createMessage(msgFullName);
    if (pMessage == nullptr) {
        LOG_WARN("Cannot find the message name:{}", msgFullName);
        return;
    }

    google::protobuf::util::JsonStringToMessage(jsonData, pMessage);

    if (!sendMsg(*pMessage)) {
        LOG_WARN("Cannot send the message:{}, json data:{}", msgFullName, jsonData);
    }
}
