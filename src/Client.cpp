#include "Client.h"
#include "Net/NetModule.h"
#include "ProtoManager.h"
#include "LuaScriptSystem.h"

#include <google/protobuf/util/json_util.h>

#include "ConfigHelper.h"
#include "MainWindow.h"

CClient::CClient(CMainWindow& window) : m_mainWin(window) {
  m_sendMsgDataBuf.reserve(MAX_BUFF_SIZE);
}

CClient::~CClient() {
  m_sendMsgDataBuf.clear();
}

bool CClient::sendMsg(const google::protobuf::Message& message) {
  size_t messageSize = message.ByteSizeLong();
  if (messageSize > m_sendMsgDataBuf.capacity()) {
    m_sendMsgDataBuf.reserve(messageSize);
  }

  if (!message.SerializeToArray(m_sendMsgDataBuf.data(), static_cast<int>(m_sendMsgDataBuf.capacity()))) {
    return false;
  }

  if (m_connection) {
    m_connection->sendProtoMsg(message.GetTypeName().c_str()
                               , m_sendMsgDataBuf.data()
                               , message.ByteSizeLong());
  }
}

void CClient::setName(const char* name) {
  m_name = name;
}

void CClient::disconnect() {
  LOG_INFO("You close connection, client: {}", getName());
  if (m_connection) {
    m_connection->disconnect();
    NetModule::instance().releaseConnection(m_connection);
    m_connection = nullptr;
  }
}

bool CClient::isConnected() {
  if (m_connection) {
    return m_connection->isConnected();
  }
  return false;
}

const char* CClient::getName() {
  return m_name.c_str();
}

void CClient::onConnectSucceed(const char* remoteIp, Port port) {
  m_mainWin.getLuaScriptSystem().Invoke("__APP_on_client_connected"
                                     , static_cast<lua_api::IClient*>(this));
  LOG_INFO("Connect succeed client: {0}, ip: {1}:{2} ", getName(), remoteIp, port);
  m_mainWin.onClientConnected(*this);
}

void CClient::onDisconnect() {
  m_mainWin.getLuaScriptSystem().Invoke("__APP_on_client_disconnected", getName());
  LOG_ERR("Connection closed, client: {}", getName());
  if (m_connection) {
    NetModule::instance().releaseConnection(m_connection);
    m_connection = nullptr;
  }
  m_mainWin.onClientDisconnect(*this);
}

void CClient::onError(ec_net::ENetError error) {
  switch (error) {
  case ec_net::eNET_CONNECT_FAIL:
    {
      LOG_ERR("Client: {} connect failed", getName());
    }
    break;
  case ec_net::eNET_SEND_OVERFLOW:
    LOG_ERR("Send buffer overflow! check \"BuffSize send\" size you've passed in \"conncet\" function on Lua script");
    break;
  case ec_net::eNET_PACKET_PARSE_FAILED:
    LOG_ERR("Packet parse failed! Packet size you returned from Lua is bigger then receive. check \"__APP_on_read_socket_buffer\" on Lua script");
    break;
  default:;
  }

  m_mainWin.onClientDisconnect(*this);
}

void CClient::onParseMessage(MessageType msgType, const char* msgFullName, const char* pData, size_t size) {
  if (msgFullName == nullptr) {
    LOG_ERR("Message name is nullptr, check lua script's function \"bindMessage\"", msgFullName);
    return;
  }

  if (nullptr == pData) {
    LOG_ERR("Message data is nullptr, check lua script's function \"bindMessage\"", msgFullName);
    return;
  }

  google::protobuf::Message* pRecvMesage = ProtoManager::instance().createMessage(msgFullName);
  if (nullptr == pRecvMesage) {
    LOG_INFO("Cannot find code of message({0}:{1})", msgFullName, msgType);
    return;
  }

  if (pRecvMesage->ParseFromArray(pData, size)) {
    m_mainWin.recvMessage(msgType, msgFullName, *pRecvMesage);
    m_mainWin.getLuaScriptSystem().Invoke("__APP_on_message_recv"
                                       , static_cast<lua_api::IClient*>(this)
                                       , msgFullName
                                       , (void*)(&pRecvMesage));
  } else {
    LOG_ERR("Protobuf message {}({}) parse failed!", msgFullName, msgType);
  }

  delete pRecvMesage;
}

void CClient::connect(const char* ip, Port port, BuffSize recv, BuffSize send) {
  if (m_connection) {
    return;
  }

  m_connection = NetModule::instance().createConnection();
  if (nullptr != m_connection) {
    m_connection->setNetEvent(this);
    m_connection->connect(ip, port, recv, send);
  }
}

void CClient::sendJsonMsg(const char* msgFullName, const char* jsonData) {
  if (!isConnected()) {
    return;
  }

  google::protobuf::Message* pMessage = ProtoManager::instance().createMessage(msgFullName);
  if (pMessage == nullptr) {
    LOG_WARN("Cannot find the message name:{}", msgFullName);
    return;
  }

  google::protobuf::util::JsonStringToMessage(jsonData, pMessage);

  if (!sendMsg(*pMessage)) {
    LOG_WARN("Cannot send the message:{}, json data:{}", msgFullName, jsonData);
  }
  delete pMessage;
}
