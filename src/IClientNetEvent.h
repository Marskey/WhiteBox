#pragma once

#include "libNetWrapper/NetEvent.h"
#include "libNetWrapper/NetPublicData.h"

class IClientNetEvent
    : public net_interface::INetEvent
    , public net_interface::IClientLinkManagerEvent
{
public:
    virtual ~IClientNetEvent() {}

    // INetEvent接口实现
    // 解析消息包
    virtual void doResolveMsg(const IMsg &msg) = 0;

    // Socket第一次连接上来触发
    virtual bool doOnConnect(const IMsg &msg, const string &strRemoteIp) = 0;

    // Socket断开连接触发
    virtual void doOnClose(SocketId unSocketID) = 0;

    // 主动连接成功后触发
    virtual void doDoConnect(SocketId unSocketID) = 0;

    // 提供网站连接 不需要解析消息头
    virtual bool doWebConnect(const IMsg &msg) = 0;

    // IClientLinkManagerEvent接口实现
    // 连接服务器失败
    virtual void onConnectFail(const std::string &strRemoteIp
                               , uint32_t unPort
                               , int32_t nConnnectType) = 0;
    // 连接服务器成功
    virtual void onConnectSuccess(const std::string &strRemoteIp
                                  , uint32_t unPort
                                  , int32_t nConnnectType
                                  , SocketId unSocketID) = 0;
};
