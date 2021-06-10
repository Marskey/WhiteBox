#pragma once

#include "INetEvent.h"

#include <asio/io_context.hpp>
#include <unordered_map>

#include "LuaScriptSystem.h"

class CSession;
typedef std::shared_ptr<CSession> SessionPtr;
class CSessionManager
{
public:
  friend class CNetModule;
  friend class CSession;
  explicit CSessionManager(asio::io_context& context);

  void connect(SocketId socketId, const char* ip, Port port, BuffSize recv, BuffSize send);
  void disconnect(SocketId socketId);
  void sendProtoMsg(SocketId socketId, const char* msgFullName, const void* pData, size_t size);

  void handleConnectSucceed(SessionPtr session);
  void handleDisconnect(SocketId socketId);
  void handleError(SocketId socketId, ec_net::ENetError error);
  void handleParseMessage(SocketId socketId, MessageType msgType, const char* fullName, const char* pData, size_t size);

private:
  void luaRegisterCppClass();
  bool loadLua(const std::string& luaScriptPath);
  CLuaScriptSystem& getLuaSystem();

private:
  asio::io_context& m_ioContext;
  std::unordered_map<SocketId, SessionPtr> m_mapSessions;
  CLuaScriptSystem m_luaSystem;
};
