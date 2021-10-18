#include "Client.h"
#include "Net/NetManager.h"
#include "ProtoManager.h"
#include "LuaScriptSystem.h"

#include <google/protobuf/util/json_util.h>

CClient::CClient() {
    m_sendMsgDataBuf.reserve(MAX_BUFF_SIZE);
}

CClient::~CClient() {
    m_sendMsgDataBuf.clear();
}

bool CClient::sendMsg(const google::protobuf::Message& message) {
    size_t messageSize = message.ByteSizeLong();
    if (messageSize > m_sendMsgDataBuf.capacity()) {
        m_sendMsgDataBuf.reserve(messageSize);
    }

    if (!message.SerializeToArray(m_sendMsgDataBuf.data(), static_cast<int>(m_sendMsgDataBuf.capacity()))) {
        return false;
    }

    return NetManager::instance().sendProtoMsg(m_socketId
                                                , message.GetTypeName().c_str()
                                                , m_sendMsgDataBuf.data()
                                                , message.ByteSizeLong());
}

SocketId CClient::getSocketID() {
    return m_socketId;
}

void CClient::setName(const char* name) {
    m_name = name;
}

void CClient::disconnect() {
    LOG_INFO("You close connection, socket id: {}", m_socketId);
    if (m_socketId != 0) {
        NetManager::instance().disconnect(m_socketId);
        m_socketId = 0;
    }
}

bool CClient::isConnected() {
    if (m_socketId != 0) {
        return NetManager::instance().isConnected(m_socketId);
    }
    return false;
}

const char* CClient::getName() {
    return m_name.c_str();
}

void CClient::onConnectSucceed(const char* remoteIp, Port port, SocketId socketId) {
    LuaScriptSystem::instance().Invoke("__APP_on_client_connected"
                                       , static_cast<lua_api::IClient*>(this));
}

void CClient::onDisconnect(SocketId socketId) {
    m_socketId = 0;
    LuaScriptSystem::instance().Invoke("__APP_on_client_disconnected", getName());
}

void CClient::onError(SocketId socketId, ec_net::ENetError error) {
}

void CClient::onParseMessage(SocketId socketId, MessageType msgType, const char* msgFullName, const char* pData, size_t size) {
}

void CClient::connect(const char* ip, Port port, BuffSize recv, BuffSize send) {
    m_socketId = NetManager::instance().connect(ip, port, recv, send);
}

void CClient::sendJsonMsg(const char* msgFullName, const char* jsonData) {
    if (!isConnected()) {
        return;
    }

    google::protobuf::Message* pMessage = ProtoManager::instance().createMessage(msgFullName);
    if (pMessage == nullptr) {
        LOG_WARN("Cannot find the message name:{}", msgFullName);
        return;
    }

    google::protobuf::util::JsonStringToMessage(jsonData, pMessage);

    if (!sendMsg(*pMessage)) {
        LOG_WARN("Cannot send the message:{}, json data:{}", msgFullName, jsonData);
    }
    delete pMessage;
}
