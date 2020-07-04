#include "Session.h"
#include "NetManager.h"
#include "LuaScriptSystem.h"

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
    return *reinterpret_cast<uint8_t*>(m_pReadData + offset);
}

int8_t CSession::CReadData::readInt8(size_t offset) {
    return *reinterpret_cast<int8_t*>(m_pReadData + offset);
}

uint16_t CSession::CReadData::readUint16(size_t offset) {
    return *reinterpret_cast<uint16_t*>(m_pReadData + offset);
}

int16_t CSession::CReadData::readInt16(size_t offset) {
    return *reinterpret_cast<int16_t*>(m_pReadData + offset);
}

uint32_t CSession::CReadData::readUint32(size_t offset) {
    return *reinterpret_cast<uint32_t*>(m_pReadData + offset);
}

int32_t CSession::CReadData::readInt32(size_t offset) {
    return *reinterpret_cast<int32_t*>(m_pReadData + offset);
}

void CSession::CReadData::parseMessage(const char* msgFullName, size_t msgStart, size_t msgSize) {
    NetManager::singleton().handleParseMessage(msgFullName, m_pReadData + msgStart, msgSize);
}

CSession::CSession(SocketId id, asio::ip::tcp::socket& s, size_t recevBuffSize, size_t sendBuffSize)
    : m_id(id)
    , m_socket(std::move(s))
    , m_sendBuffSize(sendBuffSize) {

    m_readData.m_pReadData = new char[recevBuffSize];
    m_readData.m_dataSize = recevBuffSize;
    m_readData.m_writeSize = 0;
}

CSession::~CSession() {
    close(true);

    freeSendBuf();

    delete m_readData.m_pReadData;
    m_readData.m_pReadData = nullptr;
    m_readData.m_dataSize = 0;
    m_readData.m_writeSize = 0;
}

void CSession::connect(const char* ip, Port port) {
    // 创建一个目标服务器的连接点
    asio::ip::tcp::endpoint ep(asio::ip::address_v4::from_string(ip), port);

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

    memcpy(m_ip, ip, 32);
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
    return m_ip;
}

Port CSession::getRemotePort() {
    return m_port;
}

void CSession::read() {
    if (!isValid()) {
        return;
    }

    auto asioBuffer = asio::buffer(m_readData.m_pReadData + m_readData.m_writeSize
                                   , m_readData.m_dataSize - m_readData.m_writeSize);
    try {
        m_socket.async_read_some(asioBuffer, [self = shared_from_this()](asio::error_code ec, std::size_t size) {
            if (ec) {
                // 释放内存
                self->close(true);
                return;
            }

            self->m_readData.m_writeSize += size;

            while (self->m_readData.m_writeSize > 0) {
                size_t packetSize = LuaScriptSystem::singleton().Invoke<bool>("_on_read_socket_buffer"
                                                                              , self->m_readData
                                                                              , self->m_readData.m_writeSize);
                if (0 == packetSize) {
                    break;
                }

                memmove(self->m_readData.m_pReadData
                        , self->m_readData.m_pReadData + packetSize
                        , self->m_readData.m_dataSize - packetSize);
                self->m_readData.m_writeSize -= packetSize;
            }

            self->read();
        }
        );
    } catch (std::exception& e) {
        printf("%s", e.what());
    } catch (...) {
        printf("unknown exception");
    }
}

void CSession::write() {
    if (m_sendQueue.empty()) {
        return;
    }

    if (!isValid()) {
        freeSendBuf();
        return;
    }

    CWriteData& writeData = m_sendQueue.front();
    asio::async_write(m_socket
                      , asio::buffer(writeData.data.data(), writeData.data.size())
                      , [self = shared_from_this()](std::error_code ec, std::size_t) {
        if (ec) {
            // 释放内存
            self->close(true);
            return;
        }

        if (!self->m_sendQueue.empty()) {
            self->m_sendQueue.pop_front();
            self->write();
        }
    });
}

void CSession::sendProtobufMsg(const char* msgFullName, const void* pData, size_t size) {
    CWriteData writeData;
    LuaScriptSystem::singleton().Invoke("_on_write_socket_buffer", &writeData, msgFullName, (void*)pData, size);

    if (!writeData.data.empty()) {
        if (writeData.data.size() > m_sendBuffSize) {
            handleError(ec_net::eNET_SEND_OVERFLOW);
            return;
        }

        m_sendQueue.emplace_back(writeData);
        write();
    }
}

SocketId CSession::getSocketId() {
    return m_id;
}

void CSession::handleError(ec_net::ENetError error) {
    NetManager::singleton().handleError(m_id, error);
}

void CSession::handleConnectSucceed() {
    NetManager::singleton().handleConnectSucceed(m_id);
}

void CSession::handleDisconnect() {
    NetManager::singleton().handleDisconnect(getSocketId());
}

void CSession::freeSendBuf() {
    m_sendQueue.clear();
}
