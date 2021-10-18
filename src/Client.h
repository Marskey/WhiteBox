#pragma once

#include "Net/INetEvent.h"
#include "LuaInterface.h"

#include <google/protobuf/message.h>

class CClient : public ec_net::INetEvent, public lua_api::IClient
{

public:
  CClient();
  virtual ~CClient();

  bool sendMsg(const google::protobuf::Message& message);
  SocketId getSocketID();
  void setName(const char* name);

  // ec_net::INetEvent begin
  void onConnectSucceed(const char* remoteIp, Port port, SocketId socketId) override;
  void onDisconnect(SocketId socketId) override;
  void onError(SocketId socketId, ec_net::ENetError error) override;
  void onParseMessage(SocketId socketId, MessageType msgType, const char* msgFullName, const char* pData, size_t size) override;
  // ec_net::INetEvent end

  // lua_api::IClient begin
  void connect(const char* ip, Port port, BuffSize recv, BuffSize send) override;
  void disconnect() override;
  bool isConnected() override;
  const char* getName() override;
  void sendJsonMsg(const char* msgFullName, const char* jsonData) override;
  // lua_api::IClient end
private:
  SocketId m_socketId = 0;
  std::string m_name;
  std::vector<char> m_sendMsgDataBuf;
};
