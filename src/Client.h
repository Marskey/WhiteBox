#pragma once

#include "Net/INetEvent.h"
#include "LuaInterface.h"

#include <google/protobuf/message.h>

class CMainWindow;
class CClient : public ec_net::INetEvent, public lua_api::IClient
{

public:
  CClient(CMainWindow& window);
  virtual ~CClient();

  bool sendMsg(const google::protobuf::Message& message);
  void setName(const char* name);

  // ec_net::INetEvent begin
  void onConnectSucceed(const char* remoteIp, Port port) override;
  void onDisconnect() override;
  void onError(ec_net::ENetError error) override;
  void onParseMessage(MessageType msgType, const char* msgFullName, const char* pData, size_t size) override;
  // ec_net::INetEvent end

  // lua_api::IClient begin
  void connect(const char* ip, Port port, BuffSize recv, BuffSize send) override;
  void disconnect() override;
  bool isConnected() override;
  const char* getName() override;
  void sendJsonMsg(const char* msgFullName, const char* jsonData) override;
  // lua_api::IClient end
private:
  std::string m_name;
  std::vector<char> m_sendMsgDataBuf;
  ec_net::IConnection* m_connection = nullptr;
  CMainWindow& m_mainWin;
};
