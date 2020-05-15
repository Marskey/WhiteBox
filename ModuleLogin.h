#pragma once

#include "Module.h"
#include "IClientNetEvent.h"

#include <unordered_map>
#include <functional>

#include "libProto/Client/Common/MsgDefineClient.pb.h"

class CModuleLogin : public IModule
{
public:
    CModuleLogin();
    virtual ~CModuleLogin();

    void connectLS(std::string Ip, uint16_t port, std::string account, bool isUseDevMode, int32_t kingdomId, int64_t playerId);

    void handleConnectLS(bool bResult, bool isUseDevMode, int32_t kingdomId, int64_t playerId);
    void handleConnectGS(bool bResult, uint64_t uuid, const std::string& session);

    bool isReady();
public:
    void onMessage(const ProtoMsg::MSG_LS2CL_LOGIN_RSP& rsp);
    void onMessage(const ProtoMsg::MSG_GS2CL_LOGIN_RSP& rsp);

    void onMessage(const ProtoMsg::MSG_GS2CL_LOGIN_DATA_READY_NTF& ntf);
    void onMessage(const ProtoMsg::MSG_CLIENT_KEEP_LIVE_REQ& req);

private:
    std::string m_account;
    bool m_bLoginDataReady = false;
};
