#include "ConnectionImpl.h"
#include "NetModule.h"

CConnectionImpl::CConnectionImpl(SocketId id)
  :m_id(id)
  , m_state(EConnectState::eCONNECT_STATE_NONE) {
}

CConnectionImpl::~CConnectionImpl() {
}

void CConnectionImpl::connect(const char* ip, Port port, BuffSize recv, BuffSize send) {
  if (m_state == EConnectState::eCONNECT_STATE_CONNECTING) {
    return;
  }

  NetModule::instance().connect(m_id, ip, port, recv, send);
  m_state = EConnectState::eCONNECT_STATE_CONNECTING;
}

void CConnectionImpl::disconnect() {
  m_state = EConnectState::eCONNECT_STATE_CLOSING;
  NetModule::instance().disconnect(m_id);
}

void CConnectionImpl::sendProtoMsg(const char* msgFullName, const void* pData, size_t size) {
  if (m_state == EConnectState::eCONNECT_STATE_OK) {
    NetModule::instance().sendProtoMsg(m_id, msgFullName, pData, size);
  }
}

void CConnectionImpl::setNetEvent(ec_net::INetEvent* event) {
  m_event = event;
}

SocketId CConnectionImpl::getId() {
  return m_id;
}

bool CConnectionImpl::isConnected() {
  return m_state == EConnectState::eCONNECT_STATE_OK;
}

void CConnectionImpl::onConnectSucceed(const ec_net::QNetConnectSucceedEvent& event) {
  if (m_state != EConnectState::eCONNECT_STATE_NONE) {
    printf("Connection state wrong!, id = %d", m_id);
  }

  m_state = EConnectState::eCONNECT_STATE_OK;

  if (m_event != nullptr) {
    m_event->onConnectSucceed(event.m_remoteIp.c_str(), event.m_port);
  }
}

void CConnectionImpl::onDisconnect(const ec_net::QNetDisconnectEvent& event) {
  if (m_event != nullptr) {
    m_event->onDisconnect();
  }
}

void CConnectionImpl::onError(const ec_net::QNetErrorEvent& event) {
  if (m_event != nullptr) {
    m_event->onError(event.m_error);
  }
}

void CConnectionImpl::onParseMessage(const ec_net::QNetParseMessageEvent& event) {
  if (m_event != nullptr) {
    m_event->onParseMessage(event.m_msgType, event.m_msgFullName.c_str(), event.m_pData, event.m_size);
  }
}
