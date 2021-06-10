#pragma once

#include <QEvent>

#include "define.h"

namespace ec_net
{
  enum ENetError
  {
    eNET_CONNECT_FAIL = 0,
    eNET_SEND_OVERFLOW = 3,
    eNET_PACKET_PARSE_FAILED = 4,
  };

  class INetEvent
  {
  public:
    virtual void onParseMessage(MessageType msgType, const char* msgFullName, const char* pData, size_t size) = 0;
    virtual void onConnectSucceed(const char* remoteIp, Port port) = 0;
    virtual void onDisconnect() = 0;
    virtual void onError(ENetError error) = 0;
  };

  class IConnection
  {
  public:
    virtual void connect(const char* ip, Port port, BuffSize recv, BuffSize send) = 0;
    virtual void disconnect() = 0;
    virtual void sendProtoMsg(const char* msgFullName, const void* pData, size_t size) = 0;
    virtual void setNetEvent(INetEvent* event) = 0;
    virtual SocketId getId() = 0;
    virtual bool isConnected() = 0;
  };

  class QNetConnectSucceedEvent final : public QEvent
  {
  public:
    explicit QNetConnectSucceedEvent(SocketId socketId, const std::string remoteIp, Port port)
      : QEvent(static_cast<Type>(AppEventType::eAPP_EVENT_TYPE_NET_CONNECT_SUCCEED))
      , m_socketId(socketId)
      , m_remoteIp(remoteIp)
      , m_port(port) {
    }

    SocketId m_socketId;
    std::string m_remoteIp;
    Port m_port;
  };

  class QNetDisconnectEvent final : public QEvent
  {
  public:
    explicit QNetDisconnectEvent(SocketId socketId)
      : QEvent(static_cast<Type>(AppEventType::eAPP_EVENT_TYPE_NET_DISCONNECT))
      , m_socketId(socketId) {
    }

    SocketId m_socketId;
  };

  class QNetErrorEvent final : public QEvent
  {
  public:
    explicit QNetErrorEvent(SocketId socketId, ENetError error)
      : QEvent(static_cast<Type>(AppEventType::eAPP_EVENT_TYPE_NET_ERROR))
      , m_socketId(socketId)
      , m_error(error) {
    }

    SocketId m_socketId;
    ENetError m_error;
  };

  class QNetParseMessageEvent final : public QEvent
  {
  public:
    explicit QNetParseMessageEvent(SocketId socketId, MessageType msgType, const char* msgFullName, const char* pData, size_t size)
      : QEvent(static_cast<Type>(AppEventType::eAPP_EVENT_TYPE_NET_PARSE_MESSAGE))
      , m_socketId(socketId)
      , m_msgType(msgType)
      , m_msgFullName(msgFullName)
      , m_size(size) {
      m_pData = new char[size];
      memmove(m_pData, pData, size);
    }

    ~QNetParseMessageEvent() {
      delete m_pData;
    }

    SocketId m_socketId;
    MessageType m_msgType;
    std::string m_msgFullName;
    char* m_pData;
    size_t m_size;
  };

}
