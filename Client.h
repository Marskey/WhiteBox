/********************************************************************
                 Copyright (c) 2020, IGG China R&D 3
                         All rights reserved

    创建日期： 2020年01月14日 17时00分
    文件名称： CClient.h
    说    明： 

    当前版本： 1.00
    作    者： Marskey
    概    述：

*********************************************************************/
#pragma once

#include "INetEvent.h"
#include "LuaInterface.h"

#include "google\protobuf\message.h"
#include <functional>

class CClient : public ec_net::INetEvent, public lua_api::IClient
{

public:
    CClient() = default;
    virtual ~CClient() = default;

    void connectTo(std::string ip, Port port, std::function<void(bool)> callback);

    // ec_net::INetEvent begin
    void onConnectSucceed(const char* strRemoteIp , Port port , SocketId socketId) override;
    void onDisconnect(SocketId socketId) override;
    void onError(SocketId socketId, ec_net::ENetError error) override;
    void onParseMessage(const char* msgFullName, const char* pData, size_t size) override;
    // ec_net::INetEvent end

    bool sendMsg(const google::protobuf::Message& message);

    unsigned int getSocketID();

    // lua api begin
    void connect(const char* ip, Port port, BuffSize recv, BuffSize send, const char* tag) override;
    void disconnect() override;
    void sendJsonMsg(const char* msgFullName, const char* jsonData) override;
    // lua api end
private:
    unsigned int m_socketID = 0;

    char m_msgDataBuff[60 * 1024];

    std::unordered_map<uint16_t, ::google::protobuf::Message*> m_msgs;
    std::function<void(bool)> m_connectCallback;
};
