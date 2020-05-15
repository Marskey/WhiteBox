/********************************************************************
                 Copyright (c) 2020, IGG China R&D 3
                         All rights reserved

    创建日期： 2020年01月14日 17时00分
    文件名称： ClientMessageSubject.h
    说    明： 

    当前版本： 1.00
    作    者： Marskey
    概    述：

*********************************************************************/
#pragma once

#include "IClientNetEvent.h"
#include "Module.h"

#include "google\protobuf\message.h"

#include "libCommon/Template/Singleton.h"
#include "libNetWrapper/RegisterMessageHelper.h"

class CClientMessageSubject
    : public net_interface::INetEvent
    , public net_interface::IClientLinkManagerEvent
{

public:
    CClientMessageSubject();
    virtual ~CClientMessageSubject();

public:
    void setMainListener(IClientNetEvent* obj);

    template <typename M, typename T>
    void regClientMsg(T* module) {
        M* msg = nullptr;
        auto it = m_msgs.find(M::GetStaticType());
        if (it == m_msgs.end()) {
            msg = new M();
            m_msgs[M::GetStaticType()] = msg;
        }
        else {
            msg = static_cast<M*>(it->second);
        }

        auto& callbackMap = m_listeners[M::GetStaticType()];
        callbackMap[module] = [module, msg]() { module->onMessage(*msg); };
    }

    void connectTo(std::string ip, uint16_t port, std::function<void(bool)> callback);

public:
    // INetEvent接口实现
    // 解析消息包
    virtual void doResolveMsg(const IMsg &msg) override;

    // Socket第一次连接上来触发
    virtual bool doOnConnect(const IMsg &msg, const string &strRemoteIp) override;

    // Socket断开连接触发
    virtual void doOnClose(SocketId unSocketID) override;

    // 主动连接成功后触发
    virtual void doDoConnect(SocketId unSocketID) override;

    // 提供网站连接 不需要解析消息头
    virtual bool doWebConnect(const IMsg &msg) override;

    virtual void onConnectFail(const string &strRemoteIp
                               , uint32_t unPort
                               , int32_t nConnnectType) override;
    // 连接服务器成功
    virtual void onConnectSuccess(const string &strRemoteIp
                                  , uint32_t unPort
                                  , int32_t nConnnectType
                                  , SocketId unSocketID) override;

    bool sendClientMsg(const google::protobuf::Message& message);

    unsigned int getSocketID();

private:
    NetWrapper::MsgHead *PackMsg(const google::protobuf::Message &pbMsg);

private:
    IClientNetEvent* m_mainListener;
    unsigned int m_unSocketID;

    std::map<uint16_t, std::map<IModule*, std::function<void()>>> m_listeners;
    std::unordered_map<uint16_t, ::google::protobuf::Message*> m_msgs;
    std::function<void(bool)> m_connectCallback;
};

typedef CSingleton<CClientMessageSubject> ClientMessageSubject;
