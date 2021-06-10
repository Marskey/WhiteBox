#include "SessionManager.h"

#include "NetModule.h"
#include "Session.h"

CSessionManager::CSessionManager(asio::io_context& context) : m_ioContext(context) {
}

void CSessionManager::connect(SocketId socketId, const char* ip, Port port, BuffSize recv, BuffSize send) {
  if (m_mapSessions.find(socketId) != m_mapSessions.end()) {
    return;
  }

  asio::ip::tcp::socket socket(m_ioContext);

  SessionPtr session = std::make_shared<CSession>(socketId, socket, *this, recv, send);
  session->connect(ip, port);
  m_mapSessions[session->getSocketId()] = session;
}

void CSessionManager::disconnect(SocketId socketId) {
  auto it = m_mapSessions.find(socketId);
  if (it != m_mapSessions.end()) {
    it->second->close(true);
  }
}

void CSessionManager::sendProtoMsg(SocketId socketId, const char* msgFullName, const void* pData, size_t size) {
  auto it = m_mapSessions.find(socketId);
  if (it != m_mapSessions.end()) {
    it->second->sendProtobufMsg(msgFullName, pData, size);
  }
}

void CSessionManager::handleConnectSucceed(SessionPtr session) {
  auto address = session->PeerSocketAddress();
  std::string ip;
  Port port;
  if (address.has_value()) {
    ip = address.value().address().to_string();
    port = address.value().port();
  }

  NetModule::instance().handleConnectSucceed(session->getSocketId(), ip, port);
}

void CSessionManager::handleDisconnect(SocketId socketId) {
  NetModule::instance().handleDisconnect(socketId);
  m_mapSessions.erase(socketId);
}

void CSessionManager::handleError(SocketId socketId, ec_net::ENetError error) {
  NetModule::instance().handleError(socketId, error);

  if (error == ec_net::eNET_CONNECT_FAIL) {
    m_mapSessions.erase(socketId);
  }
}

void CSessionManager::handleParseMessage(SocketId socketId, MessageType msgType, const char* fullName, const char* pData, size_t size) {
  NetModule::instance().handleParseMessage(socketId, msgType, fullName, pData, size);
}

bool CSessionManager::loadLua(const std::string& luaScriptPath) {
  m_luaSystem.Setup();
  luaRegisterCppClass();
  LOG_INFO("Initiated lua script system");

  LOG_INFO("Loading lua script: \"{}\"", luaScriptPath);
  if (!m_luaSystem.RunScript(luaScriptPath.c_str())) {
    LOG_ERR("Error: Load lua script: \"{}\" failed!", luaScriptPath);
    return false;
  }
  LOG_INFO("Lua script: \"{}\" loaded", luaScriptPath);
  return true;
}

CLuaScriptSystem& CSessionManager::getLuaSystem() {
  return m_luaSystem;
}

void CSessionManager::luaRegisterCppClass() {
  auto* pLuaState = m_luaSystem.GetLuaState();
  if (nullptr == pLuaState) {
    return;
  }
  using namespace  lua_api;
  luabridge::getGlobalNamespace(pLuaState)
    .beginClass<ISocketReader>("ISocketReader")
    .addFunction("readUint8", &ISocketReader::readUint8)
    .addFunction("readInt8", &ISocketReader::readInt8)
    .addFunction("readUint16", &ISocketReader::readUint16)
    .addFunction("readInt16", &ISocketReader::readInt16)
    .addFunction("readUint32", &ISocketReader::readUint32)
    .addFunction("readInt32", &ISocketReader::readInt32)
    .addFunction("getDataPtr", &ISocketReader::getDataPtr)
    .addFunction("bindMessage", &ISocketReader::bindMessage)
    .endClass();

  luabridge::getGlobalNamespace(pLuaState)
    .beginClass<ISocketWriter>("ISocketWriter")
    .addFunction("writeUint8", &ISocketWriter::writeUint8)
    .addFunction("writeUint8", &ISocketWriter::writeInt8)
    .addFunction("writeUint16", &ISocketWriter::writeUint16)
    .addFunction("writeInt16", &ISocketWriter::writeInt16)
    .addFunction("writeUint32", &ISocketWriter::writeUint32)
    .addFunction("writeInt32", &ISocketWriter::writeInt32)
    .addFunction("writeBinary", &ISocketWriter::writeBinary)
    .endClass();

  luabridge::getGlobalNamespace(pLuaState)
    .beginClass<CLogHelper>("Logger")
    .addFunction("log", &CLogHelper::logScriptInfo)
    .addFunction("logErr", &CLogHelper::logScriptErr)
    .endClass();

  luabridge::setGlobal(pLuaState, &(LogHelper::instance()), "Logger");
}
