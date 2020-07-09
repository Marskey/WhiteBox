#pragma once

#include "define.h"

namespace lua_api
{
    class ISocketReader
    {
    public:
        virtual ~ISocketReader() = default;
        /**
         * This function is used to read a uint8 data from socket read buffer.
         * @param offset Number of bytes to skip before starting to read. Must satisfy 0 <= offset <= buf.size - 1.
         * @returns uint8
         */
        virtual uint8_t readUint8(size_t offset) = 0;
        /**
         * This function is used to read a int8 data from socket read buffer.
         * @param offset Number of bytes to skip before starting to read. Must satisfy 0 <= offset <= buf.size - 1.
         * @returns int8
         */
        virtual int8_t readInt8(size_t offset) = 0;
        /**
         * This function is used to read a uint16 data from socket read buffer.
         * @param offset Number of bytes to skip before starting to read. Must satisfy 0 <= offset <= buf.size - 1.
         * @returns uint16
         */
        virtual uint16_t readUint16(size_t offset) = 0;
        /**
         * This function is used to read a int16 data from socket read buffer.
         * @param offset Number of bytes to skip before starting to read. Must satisfy 0 <= offset <= buf.size - 1.
         * @returns int16
         */
        virtual int16_t readInt16(size_t offset) = 0;
        /**
         * This function is used to read a uint32 data from socket read buffer.
         * @param offset Number of bytes to skip before starting to read. Must satisfy 0 <= offset <= buf.size - 1.
         * @returns uint32
         */
        virtual uint32_t readUint32(size_t offset) = 0;
        /**
         * This function is used to read a int32 data from socket read buffer.
         * @param offset Number of bytes to skip before starting to read. Must satisfy 0 <= offset <= buf.size - 1.
         * @returns int32
         */
        virtual int32_t readInt32(size_t offset) = 0;
        /**
         * This function is used to bind the message data of socket packet.
         * @param msgFullName Full name of the protobuf message.
         * @param offset Number of bytes to skip of the socket buffer.
         * @param size Length of the protobuf message data.
         */
        virtual void bindMessage(const char* msgFullName, size_t offset, size_t size) = 0;
    };

    class ISocketWriter
    {
    public:
        virtual ~ISocketWriter() = default;
        /**
         * This function is used to write a uint8 data to socket write buffer.
         * @param value the uint8 data to write
         */
        virtual void writeUint8(uint8_t value) = 0;
        /**
         * This function is used to write a int8 data to socket write buffer.
         * @param value the int8 data to write
         */
        virtual void writeInt8(int8_t value) = 0;
        /**
         * This function is used to write a uint16 data to socket write buffer.
         * @param value the uint16 data to write
         */
        virtual void writeUint16(uint16_t value) = 0;
        /**
         * This function is used to write a int16 data to socket write buffer.
         * @param value the int16 data to write
         */
        virtual void writeInt16(int16_t value) = 0;
        /**
         * This function is used to write a uint32 data to socket write buffer.
         * @param value the uint32 data to write
         */
        virtual void writeUint32(uint32_t value) = 0;
        /**
         * This function is used to write a int32 data to socket write buffer.
         * @param value the int32 data to write
         */
        virtual void writeInt32(int32_t value) = 0;
        /**
         * This function is used to write a binary data to socket write buffer.
         * @param pData data source.
         * @param size  data size.
         */
        virtual void writeBinary(void* pData, size_t size) = 0;
    };

    class IProtoManager
    {
    public:
        virtual ~IProtoManager() = default;
        /**
         * This function is used to add protobuf message that you want to show up in the application's message list.
         * @param msgType data source.
         * @param msgFullName Protobuf message full name(e.g: package_name.message_name).
         * @param msgName Protobuf message name.
         */
        virtual void addProtoMessage(int msgType, const char* msgFullName, const char* msgName) = 0;
        /**
         * This function is used to return the desciptor pool for lua script to use.
         * @returns return google::protobuf::DescriptorPool* 
         */
        virtual void* getDescriptorPool() = 0;
    };

    class IClient
    {
    public:
        virtual ~IClient() = default;
        /**
         * This function is used to connect to specify remote endpoint.
         * @param ip remote ip4 address.
         * @param port remote port.
         * @param recv socket receive buffer size
         * @param send socket send buffer size
         */
        virtual void connect(const char* ip, Port port, BuffSize recv, BuffSize send) = 0;
        /**
         * This function is used to disconnect to specify remote endpoint.
         */
        virtual void disconnect() = 0;
        virtual bool isConnected() = 0;
        virtual const char* getName() = 0;
        /**
         * This function is used to send protobuf message data to remote endpoint.
         * @param msgFullName protobuf message full name.
         * @param jsonData JSON format of protobuf message data.
         */
        virtual void sendJsonMsg(const char* msgFullName, const char* jsonData) = 0;
    };

    /**
     * This is main application instance. will always alive through all to the end.
     */
    class IMainApp
    {
    public:
        virtual ~IMainApp() = default;
        /**
         * This function is used to add timer
         * @param interval milliseconds 
         * @returns return timer id 
         */
        virtual int addTimer(int interval) = 0;
        /**
         * This function is used to delete timer
         * @param timerId 
         */
        virtual void deleteTimer(int timerId) = 0;

        virtual IClient* createClient(const char* name) = 0;

        virtual IClient* getClient(const char* name) = 0;
    };
}
