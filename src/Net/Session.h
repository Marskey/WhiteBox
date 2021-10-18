#pragma once

#include "INetEvent.h"
#include "LuaInterface.h"

#include <asio/ip/tcp.hpp>
#include <deque>

class CSession : public std::enable_shared_from_this<CSession>
{
  class CWriteData final : public lua_api::ISocketWriter
  {
  public:
    std::vector<char> data;

    void writeUint8(uint8_t value) override;
    void writeInt8(int8_t value) override;
    void writeUint16(uint16_t value) override;
    void writeInt16(int16_t value) override;
    void writeUint32(uint32_t value) override;
    void writeInt32(int32_t value) override;
    void writeBinary(void* pData, size_t size) override;
  };

  class CReadData final : public lua_api::ISocketReader
  {
  public:
    virtual ~CReadData() {
      if (pReadData != nullptr) {
        delete[] pReadData;
        pReadData = nullptr;
      }
    }

    char* pReadData = nullptr;
    size_t dataSize = 0;
    size_t writeSize = 0;

    MessageType messageType = 0;
    const char* messageFullName = "";
    const char* pMessageData = nullptr;
    size_t messageSize = 0;

    uint8_t readUint8(size_t offset) override;
    int8_t readInt8(size_t offset) override;
    uint16_t readUint16(size_t offset) override;
    int16_t readInt16(size_t offset) override;
    uint32_t readUint32(size_t offset) override;
    int32_t readInt32(size_t offset) override;
    void* getDataPtr(size_t offset) override;
    void bindMessage(MessageType msgType, const char* msgFullName, void* pData, size_t size) override;
  };

public:
  explicit CSession(SocketId id, asio::ip::tcp::socket& s, size_t recevBuffSize, size_t sendBuffSize);
  virtual ~CSession();

  void connect(const char* ip, Port port);
  bool isValid();
  void close(bool notice);
  const char* getRemoteIP();
  Port getRemotePort();
  void read();
  void write();
  void sendProtobufMsg(const char* msgFullName, const void* pData, size_t size);
  SocketId getSocketId();

private:
  void handleError(ec_net::ENetError error);
  void handleConnectSucceed();
  void handleDisconnect();

  void freeSendBuf();

private:
  SocketId m_id = 0;
  asio::ip::tcp::socket m_socket;
  std::deque<CWriteData*> m_sendQueue;
  CReadData m_readData;

  size_t m_sendBuffSize = 0;

  std::string m_ip = "111.111.111.111";
  Port m_port;
};
