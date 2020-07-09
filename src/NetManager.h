#pragma once

#include "INetEvent.h"

#include <asio/io_context.hpp>
#include <unordered_map>

#include "Singleton.h"

class CSession;
typedef std::shared_ptr<CSession> SessionPtr;
class CNetManager
{
public:
    CNetManager();
    virtual ~CNetManager();

    bool init(ec_net::INetEvent* pEvent);
    void run(int count = -1);
    void stop();
    SocketId connect(const char* ip, Port port, BuffSize recv, BuffSize send);
    void disconnect(SocketId socketId);

    bool isConnected(SocketId socketId);

    bool sendProtoMsg(SocketId socketId, const char* msgFullName, const void* pData, size_t size);

    std::string getRemoteIP(SocketId socketId);
    Port getRemotePort(SocketId socketId);

    void handleConnectSucceed(SocketId socketId);
    void handleDisconnect(SocketId socketId);
    void handleError(SocketId socketId, ec_net::ENetError error);

    void handleParseMessage(SocketId socketId, const char* fullName, const char* pData, size_t size);

    void addSession(SocketId socketId, SessionPtr session);
private:
    SocketId genSocketId();

private:
    ec_net::INetEvent* m_pNetEvent = nullptr;
    asio::io_context m_ioContext;

    // 从0 开始自增
    SocketId m_incrediId = 0;
    std::unordered_map<SocketId, SessionPtr> m_mapSessions;
};

typedef CSingleton<CNetManager> NetManager;
