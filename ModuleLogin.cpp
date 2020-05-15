#include "ModuleLogin.h"
#include "ClientMessageSubject.h"
#include "libNetWrapper/NetService.h"

CModuleLogin::CModuleLogin() {
    ClientMessageSubject::singleton().regClientMsg<ProtoMsg::MSG_LS2CL_LOGIN_RSP>(this);
    ClientMessageSubject::singleton().regClientMsg<ProtoMsg::MSG_GS2CL_LOGIN_RSP>(this);
    ClientMessageSubject::singleton().regClientMsg<ProtoMsg::MSG_GS2CL_LOGIN_DATA_READY_NTF>(this);
    ClientMessageSubject::singleton().regClientMsg<ProtoMsg::MSG_CLIENT_KEEP_LIVE_REQ>(this);
}

CModuleLogin::~CModuleLogin() {
}

void CModuleLogin::connectLS(std::string Ip, uint16_t port, std::string account, bool isUseDevMode, int32_t kingdomId, int64_t playerId) {
    m_account = account;

    ClientMessageSubject::singleton().connectTo(Ip
                                                , port
                                                , std::bind(&CModuleLogin::handleConnectLS, this, std::placeholders::_1, isUseDevMode, kingdomId, playerId));
}

void CModuleLogin::handleConnectLS(bool bResult, bool isUseDevMode, int32_t kingdomId, int64_t playerId) {
    if (!bResult) {
        return;
    }


    // 注册
    {
        ProtoMsg::MSG_SERVER_REGIST_REQ req;
        req.mutable_info()->set_server_type(1/*EServerType::eSERVERTYPE_CLIENT*/);
        ClientMessageSubject::singleton().sendClientMsg(req);
    }

    {
        ProtoMsg::MSG_CL2LS_LOGIN_REQ req;
        req.set_account(m_account);
        req.set_want_kingdom_id(1);
        req.set_login_for_dev(isUseDevMode);
        req.set_player_id(playerId);
        req.set_select_kingdom_id(kingdomId);
        ClientMessageSubject::singleton().sendClientMsg(req);
    }
}

void CModuleLogin::handleConnectGS(bool bResult, uint64_t uuid, const std::string& session) {
    if (!bResult) {
        return;
    }

    // 注册
    {
        ProtoMsg::MSG_SERVER_REGIST_REQ req;
        req.mutable_info()->set_server_type(1/*EServerType::eSERVERTYPE_CLIENT*/);
        ClientMessageSubject::singleton().sendClientMsg(req);
    }

    {
        ProtoMsg::MSG_CL2GS_LOGIN_REQ req;
        req.set_player_id(uuid);
        req.set_account(m_account);
        req.set_login_session(session);
        ClientMessageSubject::singleton().sendClientMsg(req);
    }
}

bool CModuleLogin::isReady() {
    return m_bLoginDataReady;
}

void CModuleLogin::onMessage(const ProtoMsg::MSG_LS2CL_LOGIN_RSP& rsp) {
    NetService::singleton().disConnect(ClientMessageSubject::singleton().getSocketID());
    std::string ip = rsp.net_addr().ip().c_str();
    uint16_t port = rsp.net_addr().port();


    ClientMessageSubject::singleton().connectTo(ip
                                                , port
                                                , std::bind(&CModuleLogin::handleConnectGS, this, std::placeholders::_1, rsp.player_id(), rsp.login_session()));
}

void CModuleLogin::onMessage(const ProtoMsg::MSG_GS2CL_LOGIN_RSP& rsp) {
    if (rsp.ret_code() != EMsgErrorCode::eMEC_SUCCESS) {
        NetService::singleton().disConnect(ClientMessageSubject::singleton().getSocketID());
        return;
    }
}

void CModuleLogin::onMessage(const ProtoMsg::MSG_GS2CL_LOGIN_DATA_READY_NTF& ntf) {
    m_bLoginDataReady = true;
}

void CModuleLogin::onMessage(const ProtoMsg::MSG_CLIENT_KEEP_LIVE_REQ& req) {
    ProtoMsg::MSG_CLIENT_KEEP_LIVE_RSP rsp;
    ClientMessageSubject::singleton().sendClientMsg(rsp);
}

