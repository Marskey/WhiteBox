#pragma once

#include "define.h"

namespace ec_net
{
    enum ENetError
    {
        eNET_CONNECT_FAIL = 0,
        eNET_SEND_OVERFLOW = 3,
        eNET_PARSER_NULL = 4,
    };

    class INetEvent
    {
    public:
        virtual void onParseMessage(const char* msgFullName, const char* pData, size_t size) = 0;
        virtual void onConnectSucceed(const char* strRemoteIp
                                      , Port unPort
                                      , SocketId socketId) = 0;
        virtual void onDisconnect(SocketId socketId) = 0;
        virtual void onError(SocketId socketId, ENetError error) = 0;
    };
}
