#include "NetModule.h"

#include "ConfigHelper.h"
#include "ConnectionImpl.h"

bool CNetModule::start() {
  QString luaScriptPath = ConfigHelper::instance().getWidgetComboBoxStateText("LuaScriptPath", 0);
  bool ret = m_sessionMgr.loadLua(luaScriptPath.toStdString());
  if (!ret) {
    return false;
  }
  
  m_asioThead = std::make_unique<std::thread>([this]() {
    auto work = std::make_shared<asio::io_context::work>(m_ioContext);
    m_ioContext.run();
                                              });
  return true;
}

void CNetModule::stop() {
  if (m_asioThead == nullptr) {
    return;
  }

  postToNet([this] {
    m_ioContext.stop();
            });

  postToNet([this] {
    m_sessionMgr.getLuaSystem().Shutdown();
            });

  m_asioThead->join();
  m_asioThead.reset();
}

void CNetModule::connect(SocketId socketId, const char* ip, Port port, BuffSize recv, BuffSize send) {
  postToNet([this, socketId, ip, port, recv, send] {
    m_sessionMgr.connect(socketId, ip, port, recv, send);
            });
}

void CNetModule::disconnect(SocketId socketId) {
  postToNet([this, socketId] {
    m_sessionMgr.disconnect(socketId);
            });
}

void CNetModule::sendProtoMsg(SocketId socketId, std::string msgFullName, const void* pData, size_t size) {
  postToNet([this, socketId, msgFullName, pData, size] {
    m_sessionMgr.sendProtoMsg(socketId, msgFullName.c_str(), pData, size);
            });
}

void CNetModule::postToApp(QEvent* event, int priority) {
  QApplication::postEvent(this, event, priority);
}

bool CNetModule::event(QEvent* event) {
  switch (static_cast<AppEventType>(event->type())) {
  case AppEventType::eAPP_EVENT_TYPE_NET_CONNECT_SUCCEED:
    {
      auto* e = static_cast<ec_net::QNetConnectSucceedEvent*>(event);
      auto it = m_mapConnections.find(e->m_socketId);
      if (it != m_mapConnections.end()) {
        it->second->onConnectSucceed(*e);
      }
    }
    break;
  case AppEventType::eAPP_EVENT_TYPE_NET_DISCONNECT:
    {
      auto* e = static_cast<ec_net::QNetDisconnectEvent*>(event);
      auto it = m_mapConnections.find(e->m_socketId);
      if (it != m_mapConnections.end()) {
        it->second->onDisconnect(*e);
      }
    }
    break;
  case AppEventType::eAPP_EVENT_TYPE_NET_ERROR:
    {
      auto* e = static_cast<ec_net::QNetErrorEvent*>(event);
      auto it = m_mapConnections.find(e->m_socketId);
      if (it != m_mapConnections.end()) {
        it->second->onError(*e);
      }
    }
    break;
  case AppEventType::eAPP_EVENT_TYPE_NET_PARSE_MESSAGE:
    {
      auto* e = static_cast<ec_net::QNetParseMessageEvent*>(event);
      auto it = m_mapConnections.find(e->m_socketId);
      if (it != m_mapConnections.end()) {
        it->second->onParseMessage(*e);
      }
    }
    break;
  default:;
  }
  return true;
}

void CNetModule::handleConnectSucceed(SocketId socketId, const std::string& ip, Port port) {
  postToApp(new ec_net::QNetConnectSucceedEvent{socketId, ip, port});
}

void CNetModule::handleDisconnect(SocketId socketId) {
  postToApp(new ec_net::QNetDisconnectEvent{socketId});
}

void CNetModule::handleError(SocketId socketId, ec_net::ENetError error) {
  postToApp(new ec_net::QNetErrorEvent{socketId, error});
}

void CNetModule::handleParseMessage(SocketId socketId, MessageType msgType, const char* fullName, const char* pData, size_t size) {
  postToApp(new ec_net::QNetParseMessageEvent{ socketId, msgType, fullName, pData, size });
}

ec_net::IConnection* CNetModule::createConnection() {
  auto* pConnection = new CConnectionImpl(++m_increaseId);
  m_mapConnections[pConnection->getId()] = pConnection;
  return pConnection;
}

void CNetModule::releaseConnection(ec_net::IConnection* connection) {
  if (nullptr == connection) {
    return;
  }

  m_mapConnections.erase(connection->getId());
  delete connection;
}
