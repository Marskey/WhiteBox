#include "Client.h"
#include "toolkit/fmt/format.h"
#include "NetManager.h"
#include "ProtoManager.h"
#include "LuaScriptSystem.h"

#include <google/protobuf/util/json_util.h>

void CClient::connectTo(std::string ip, Port port, std::function<void(bool)> callback) {
    m_connectCallback = callback;

    NetManager::singleton().connect(ip.c_str(), port, 131073,131073);
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

bool CClient::sendMsg(const google::protobuf::Message& message) {
    // TODO (Marskey): 修改一下
    if (!message.SerializeToArray(m_msgDataBuff, 60 * 1024)) {
        return false;
    }

    return NetManager::singleton().sendProtoMsg(m_socketID
                                                , message.GetTypeName().c_str()
                                                , m_msgDataBuff
                                                , message.ByteSizeLong());
}

void CClient::connect(const char* ip, Port port, BuffSize recv, BuffSize send, const char* tag) {
    NetManager::singleton().connect(ip, port, recv, send);
    m_connectCallback = [this, t = std::string(tag)] (bool bResult) {
        if (!bResult) {
            return;
        }

        LuaScriptSystem::singleton().Invoke("_on_client_connected", this, t);
    };
}

void CClient::sendJsonMsg(const char* msgFullName, const char* jsonData) {
    google::protobuf::Message* pMessage = ProtoManager::singleton().createMessage(msgFullName);
    if (pMessage == nullptr) {
        // TODO (Marskey): 修改一下
        return;
    }

    google::protobuf::util::JsonStringToMessage(jsonData, pMessage);

    // TODO (Marskey): 错误日志
    if (!sendMsg(*pMessage)) {
    }
}

unsigned int CClient::getSocketID() {
    return m_socketID;
}
