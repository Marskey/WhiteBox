#include "NetManager.h"
#include "Session.h"

#include <QDebug>

#include <asio/ip/tcp.hpp>


CNetManager::CNetManager() {
}

CNetManager::~CNetManager() {
    m_pNetEvent = nullptr;
}

bool CNetManager::init(ec_net::INetEvent* pEvent) {
    m_pNetEvent = pEvent;

    return true;
}

void CNetManager::run(int count /*= -1*/) {
    if (count > 0) {
        for (int32_t i = 0; i < count; ++i) {
            m_ioContext.run_one();
        }
    } else {
        m_ioContext.run();
    }
}

void CNetManager::stop() {
    m_ioContext.stop();
}

void CNetManager::connect(const char* ip, Port port, BuffSize recv, BuffSize send) {
	asio::ip::tcp::socket socket(m_ioContext);

    SessionPtr session = SessionPtr(new CSession(genSocketId(), socket, recv, send));
    session->connect(ip, port);

    m_mapSessions[session->getSocketId()] = session;
}

void CNetManager::disconnect(SocketId socketId) {
    auto it = m_mapSessions.find(socketId);
    if (it != m_mapSessions.end()) {
        it->second->close(true);
    }
}

bool CNetManager::isConnected(SocketId socketId) {
    auto it = m_mapSessions.find(socketId);
    if (it != m_mapSessions.end()) {
        return it->second->isValid();
    }

    return false;
}

bool CNetManager::sendProtoMsg(SocketId socketId, const char* msgFullName, const void* pData, size_t size) {
    auto it = m_mapSessions.find(socketId);
    if (it != m_mapSessions.end()) {
        it->second->sendProtobufMsg(msgFullName, pData, size);
        return true;
    }

    return false;
}

std::string CNetManager::getRemoteIP(SocketId socketId) {
    auto it = m_mapSessions.find(socketId);
    if (it != m_mapSessions.end()) {
        return it->second->getRemoteIP();
    }

    return "";
}

Port CNetManager::getRemotePort(SocketId socketId) {
    auto it = m_mapSessions.find(socketId);
    if (it != m_mapSessions.end()) {
        return it->second->getRemotePort();
    }

    return 0;
}

void CNetManager::handleConnectSucceed(SocketId socketId) {
    if (m_pNetEvent) {
        auto it = m_mapSessions.find(socketId);
        if (it != m_mapSessions.end()) {
            m_pNetEvent->onConnectSucceed(it->second->getRemoteIP(), it->second->getRemotePort(), socketId);
        }
    }
}

void CNetManager::handleDisconnect(SocketId socketId) {
    if (m_pNetEvent) {
        m_pNetEvent->onDisconnect(socketId);
    }

    m_mapSessions.erase(socketId);
}

void CNetManager::handleError(SocketId socketId, ec_net::ENetError error) {
    if (m_pNetEvent) {
        m_pNetEvent->onError(socketId, error);
    }

    if (error != ec_net::eNET_SEND_OVERFLOW) {
        m_mapSessions.erase(socketId);
    }
}

void CNetManager::handleParseMessage(const char* fullName, const char* pData, size_t size) {
    if (m_pNetEvent) {
        m_pNetEvent->onParseMessage(fullName, pData, size);
    }
}

SocketId CNetManager::genSocketId() {
    return ++m_incrediId;
}

