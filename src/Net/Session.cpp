#include "Session.h"
#include "NetManager.h"
#include "src/LuaScriptSystem.h"

#include <asio/read.hpp>
#include <asio/write.hpp>

void CSession::CWriteData::writeUint8(uint8_t value) {
  size_t size = sizeof(uint8_t);
  size_t offset = data.size();
  data.resize(data.size() + size);
  memcpy(reinterpret_cast<char*>(data.data()) + offset, &value, size);
}

void CSession::CWriteData::writeInt8(int8_t value) {
  size_t size = sizeof(int8_t);
  size_t offset = data.size();
  data.resize(data.size() + size);
  memcpy(reinterpret_cast<char*>(data.data()) + offset, &value, size);
}

void CSession::CWriteData::writeUint16(uint16_t value) {
  size_t size = sizeof(uint16_t);
  size_t offset = data.size();
  data.resize(data.size() + size);
  memcpy(reinterpret_cast<char*>(data.data()) + offset, &value, size);
}

void CSession::CWriteData::writeInt16(int16_t value) {
  size_t size = sizeof(int16_t);
  size_t offset = data.size();
  data.resize(data.size() + size);
  memcpy(reinterpret_cast<char*>(data.data()) + offset, &value, size);
}

void CSession::CWriteData::writeUint32(uint32_t value) {
  size_t size = sizeof(uint32_t);
  size_t offset = data.size();
  data.resize(data.size() + size);
  memcpy(reinterpret_cast<char*>(data.data()) + offset, &value, size);
}

void CSession::CWriteData::writeInt32(int32_t value) {
  size_t size = sizeof(int32_t);
  size_t offset = data.size();
  data.resize(data.size() + size);
  memcpy(reinterpret_cast<char*>(data.data()) + offset, &value, size);
}

void CSession::CWriteData::writeBinary(void* pData, size_t size) {
  size_t offset = data.size();
  data.resize(data.size() + size);
  memcpy(reinterpret_cast<char*>(data.data()) + offset, pData, size);
}

uint8_t CSession::CReadData::readUint8(size_t offset) {
  return *reinterpret_cast<uint8_t*>(pReadData + offset);
}

int8_t CSession::CReadData::readInt8(size_t offset) {
  return *reinterpret_cast<int8_t*>(pReadData + offset);
}

uint16_t CSession::CReadData::readUint16(size_t offset) {
  return *reinterpret_cast<uint16_t*>(pReadData + offset);
}

int16_t CSession::CReadData::readInt16(size_t offset) {
  return *reinterpret_cast<int16_t*>(pReadData + offset);
}

uint32_t CSession::CReadData::readUint32(size_t offset) {
  return *reinterpret_cast<uint32_t*>(pReadData + offset);
}

int32_t CSession::CReadData::readInt32(size_t offset) {
  return *reinterpret_cast<int32_t*>(pReadData + offset);
}

void* CSession::CReadData::getDataPtr(size_t offset) {
  return reinterpret_cast<void*>(pReadData + offset);
}

void CSession::CReadData::bindMessage(MessageType msgType, const char* msgFullName, void* pData, size_t size) {
  messageType = msgType;
  messageFullName = msgFullName;
  pMessageData = static_cast<const char*>(pData);
  messageSize = size;
}

CSession::CSession(SocketId id, asio::ip::tcp::socket& s, asio::ip::tcp::resolver& r, size_t recevBuffSize, size_t sendBuffSize)
  : m_id(id)
  , m_socket(std::move(s))
  , m_resolver(std::move(r))
  , m_sendBuffSize(sendBuffSize) {

  m_readData.pReadData = new char[recevBuffSize];
  m_readData.dataSize = recevBuffSize;
  m_readData.writeSize = 0;
}

CSession::~CSession() {
  close(true);
  freeSendBuf();
  m_socket.release();
}

void CSession::connect(const char* ip, Port port) {
  // 创建一个目标服务器的连接点
  asio::error_code ec;
  auto ipAddress = asio::ip::address_v4::from_string(ip, ec);
  asio::ip::tcp::endpoint ep;
  if (!ec) {
    ep = { ipAddress, port };
  } else {
    asio::ip::tcp::resolver::query query(ip, std::to_string(port));
    asio::ip::tcp::resolver::iterator it =
      m_resolver.resolve(query, ec);

    if (!ec) {
      ep = it->endpoint();
    }
  }

  if (isValid()) {
    return;
  }

  m_socket.async_connect(ep, [self = shared_from_this()](auto ec) {
    if (ec) {
      self->handleError(ec_net::eNET_CONNECT_FAIL);
      return;
    }

    // 设置 nodelay
    self->m_socket.set_option(asio::ip::tcp::no_delay(true));

    // 处理连接成功
    self->handleConnectSucceed();
    // 开始读数据
    self->read();
  });

  m_ip = ip ;
  m_port = port;
}

bool CSession::isValid() {
  return m_socket.is_open();
}

void CSession::close(bool notice) {
  if (m_socket.is_open()) {
    std::error_code ec;
    m_socket.close(ec);
    if (ec) {
      // TODO (Marskey): 错误日志
    }

    if (notice) {
      handleDisconnect();
    }
  }
}

const char* CSession::getRemoteIP() {
  return m_ip.c_str();
}

Port CSession::getRemotePort() {
  return m_port;
}

void CSession::read() {
  if (!isValid()) {
    return;
  }

  auto asioBuffer = asio::buffer(m_readData.pReadData + m_readData.writeSize
                                 , m_readData.dataSize - m_readData.writeSize);
  try {
    m_socket.async_read_some(asioBuffer, [self = shared_from_this()](asio::error_code ec, std::size_t size) {
      if (ec) {
        // 释放内存
        self->close(true);
        return;
      }

      self->m_readData.writeSize += size;

      while (self->m_readData.writeSize > 0) {
        auto packetSize = LuaScriptSystem::instance().Invoke<size_t>("__APP_on_read_socket_buffer"
                                                                     , static_cast<lua_api::ISocketReader*>(&self->m_readData)
                                                                     , self->m_readData.writeSize);

        if (packetSize > self->m_readData.writeSize) {
          break;
        }

        if (0 == packetSize) {
          break;
        }

        NetManager::instance().handleParseMessage(self->m_id
                                                  , self->m_readData.messageType
                                                  , self->m_readData.messageFullName
                                                  , self->m_readData.pMessageData
                                                  , self->m_readData.messageSize);

        memmove(self->m_readData.pReadData
                , self->m_readData.pReadData + packetSize
                , self->m_readData.writeSize - packetSize);

        self->m_readData.writeSize -= packetSize;
      }

      self->write();
    }
    );
  }
  catch (std::exception& e) {
    printf("%s\n", e.what());
  }
  catch (...) {
    printf("unknown exception\n");
  }
}

void CSession::write() {
  if (m_sendQueue.empty()) {
    read();
    return;
  }

  if (!isValid()) {
    freeSendBuf();
    return;
  }

  auto* pWriteData = m_sendQueue.front();
  m_sendQueue.pop_front();
  try {
    asio::async_write(m_socket
                      , asio::buffer(pWriteData->data.data(), pWriteData->data.size())
                      , [self = shared_from_this(), pWriteData](std::error_code ec, std::size_t size) {
      delete pWriteData;
      if (ec) {
        // 释放内存
        self->close(true);
        return;
      }

      if (!self->m_sendQueue.empty()) {
        self->write();
      }
    });
  }
  catch (std::exception& e) {
    printf("%s\n", e.what());
  }
  catch (...) {
    printf("unknown exception\n");
  }
}

void CSession::sendProtobufMsg(const char* msgFullName, const void* pData, size_t size) {
  CWriteData* writeData = new CWriteData;
  LuaScriptSystem::instance().Invoke("__APP_on_write_socket_buffer"
                                     , static_cast<lua_api::ISocketWriter*>(writeData)
                                     , msgFullName
                                     , (void*)pData, size);

  if (writeData->data.empty()) {
    delete writeData;
    return;
  }

  if (writeData->data.size() > m_sendBuffSize) {
    handleError(ec_net::eNET_SEND_OVERFLOW);
    delete writeData;
    return;
  }

  m_sendQueue.emplace_back(writeData);
  write();
}

SocketId CSession::getSocketId() {
  return m_id;
}

void CSession::handleError(ec_net::ENetError error) {
  NetManager::instance().handleError(m_id, error);
}

void CSession::handleConnectSucceed() {
  NetManager::instance().handleConnectSucceed(m_id);
}

void CSession::handleDisconnect() {
  NetManager::instance().handleDisconnect(getSocketId());
}

void CSession::freeSendBuf() {
  for (int i = 0; i < m_sendQueue.size(); ++i) {
    delete m_sendQueue[i];
  }
  m_sendQueue.clear();
}
