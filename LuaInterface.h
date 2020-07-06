#pragma once

#include "define.h"

namespace lua_api
{
    class IReadData
    {
    public:
        virtual ~IReadData() = default;
        virtual uint8_t readUint8(size_t offset) = 0;
        virtual int8_t readInt8(size_t offset) = 0;
        virtual uint16_t readUint16(size_t offset) = 0;
        virtual int16_t readInt16(size_t offset) = 0;
        virtual uint32_t readUint32(size_t offset) = 0;
        virtual int32_t readInt32(size_t offset) = 0;
        virtual void parseMessage(const char* msgFullName, size_t msgStart, size_t msgSize) = 0;
    };

    class IWriteData
    {
    public:
        virtual ~IWriteData() = default;
        virtual void writeUint8(uint8_t value) = 0;
        virtual void writeInt8(int8_t value) = 0;
        virtual void writeUint16(uint16_t value) = 0;
        virtual void writeInt16(int16_t value) = 0;
        virtual void writeUint32(uint32_t value) = 0;
        virtual void writeInt32(int32_t value) = 0;
        virtual void writeBinary(void* pData, size_t size) = 0;
    };

    class IProtoManager
    {
    public:
        virtual ~IProtoManager() = default;
        virtual void addProtoMessage (int msgType, const char* msgFullName, const char* msgName) = 0;
        virtual void* getDescriptorPool() = 0;
    };

    class IClient
    {
    public:
        virtual ~IClient() = default;
        virtual void connect(const char* ip, Port port, BuffSize recv, BuffSize send, const char* tag) = 0;
        virtual void disconnect() = 0;
        virtual void sendJsonMsg(const char* msgFullName, const char* jsonData) = 0;
    };
}
