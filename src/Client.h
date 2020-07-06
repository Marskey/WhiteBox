#pragma once

#include "INetEvent.h"
#include "LuaInterface.h"

#include "google\protobuf\message.h"
#include <functional>

class CClient : public ec_net::INetEvent, public lua_api::IClient
{

public:
    CClient();
    virtual ~CClient() = default;

    // 暂留 //
    void connectTo(std::string ip, Port port, std::function<void(bool)> callback);

    bool sendMsg(const google::protobuf::Message& message);
    unsigned int getSocketID();

    // ec_net::INetEvent begin
    void onConnectSucceed(const char* strRemoteIp , Port port , SocketId socketId) override;
    void onDisconnect(SocketId socketId) override;
    void onError(SocketId socketId, ec_net::ENetError error) override;
    void onParseMessage(const char* msgFullName, const char* pData, size_t size) override;
    // ec_net::INetEvent end

    // lua_api::IClient begin
    void connect(const char* ip, Port port, BuffSize recv, BuffSize send, const char* tag) override;
    void disconnect() override;
    void sendJsonMsg(const char* msgFullName, const char* jsonData) override;
    // lua_api::IClient end
private:
    unsigned int m_socketID = 0;
    std::vector<char> m_sendMsgDataBuf;

    std::unordered_map<uint16_t, ::google::protobuf::Message*> m_msgs;
    std::function<void(bool)> m_connectCallback;
};
