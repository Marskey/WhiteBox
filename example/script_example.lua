package.path = package.path..";./script/?.lua"

local get_full_name_by_type = {}
local get_type_by_full_name = {}

local g_reserve_data = {}

-- 定义包头大小
local msg_head_size = 6

local json = require "json"

-- __APP_on_proto_reload
-- 每当客户端加载完proto文件后被调用
-- @param ProtoMananger Cpp类的实例指针，注册函数看文本最后
function __APP_on_proto_reload(IProtoManager)
    -- 以下是范例
    local pool = IProtoManager:getDescriptorPool()
    if pool == nil then
        return
    end
    local enum_descriptor = protobuf.descriptorpool_findenumtypebyname(pool, "EMsgTypes")
    if enum_descriptor == nil then
        return
    end
    local count = protobuf.enumdescriptor_valuecount(enum_descriptor)
    for i = 0, count - 1, 1 do
        local enumvalue_descriptor = protobuf.enumdescriptor_value(enum_descriptor, i)
        if enumvalue_descriptor ~= nil then
            local field_name = protobuf.enumvaluedescriptor_name(enumvalue_descriptor)
            local field_number = protobuf.enumvaluedescriptor_number(enumvalue_descriptor)

            -- EMsgTypes中是 _MSG_XXX_XXX_REQ = 2000 这样的，所以要去掉前面的'_'来获取消息名
            local msg_name = string.sub(field_name, 2)

            -- ProtoMsg是package的名字
            local full_name = "ProtoMsg." .. msg_name
            if string.find(full_name, "2CL") ~= nil or string.find(full_name, "CL2") ~= nil then

                get_full_name_by_type[field_number] = full_name
                get_type_by_full_name[full_name] = field_number

                -- message type 定义在了EMsgTypes所以这个field_number就是相当于消息协议中的message id
                -- 如果没有 message type 可以填0（不会吧，不会真的有人没有定义message type吧，不会吧）
                IProtoManager:addProtoMessage(field_number, full_name, msg_name)
            end
        end
    end
    return
end

-- __APP_on_read_socket_buffer
-- 每当客户端接收到网络数据包后被调用, 需要在该函数中对数据块进行拆包操作（解决粘包问题）
-- @param ISocketReader Cpp IRead类指针 网络数据包块，注册函数看文本最后
-- @param buff_size 网络数据包块的大小
-- @returns packet_size 返回拆包后一个包的大小，如果返回0则会继续等待数据包到来后被调用
function __APP_on_read_socket_buffer(ISocketReader, buff_size)
    -- 以下是范例

    if buff_size < msg_head_size then
        return 0
    end

    -- 假设消息包头是这样的结构
    -- struct MsgHeader {
    --  uint16 packetSize;
    --  uint16 msgType;
    --  uint8 flag;
    --  uint8 opt;
    -- }
    -- 可知包头大小是6

    local packet_size = ISocketReader:readUint16(0)
    if packet_size <= 0 then
        return 0
    end

    local msg_type = ISocketReader:readUint16(2)
    if msg_type == 0 then
        return 0
    end

    local flag = ISocketReader:readUint8(4)

    local opt = ISocketReader:readUint8(5)

    local msg_full_name = get_full_name_by_type[msg_type]
    local msg_size = packet_size - msg_head_size

    -- 用bindMessage来告诉应用程序需要解析的消息结构，因为消息数据在包头之后
    -- ，所以第二个参数传入的是偏移一个包头大小的字节数

    ISocketReader:bindMessage(msg_full_name, 0 + msg_head_size, msg_size)
    return packet_size
end

-- __APP_on_write_socket_buffer
-- 每当客户端发送数据包前被调用，需要在此实现对数据包头的添加
-- @param ISocketReader Cpp ISendData类指针 待发送的网络数据包块，注册函数看文本最后
-- @param msg_full_name 待发送的protobuf message的全名（package name.message）
-- @param protobuf_data protobuf message cpp数据地址指针
-- @param protobuf_size protobuf message 数据大小
function __APP_on_write_socket_buffer(ISocketReader, msg_full_name, protobuf_data, protobuf_size)
    -- 以下是范例
    -- 此范例展示了如何向socket缓冲区写入包数据
    local msg_type = get_type_by_full_name[msg_full_name]
    if msg_type == nil then
        return
    end
    ISocketReader:writeUint16(protobuf_size + msg_head_size)
    ISocketReader:writeUint16(msg_type)
    ISocketReader:writeUint8(0)
    ISocketReader:writeUint8(1)
    ISocketReader:writeBinary(protobuf_data, protobuf_size)
end

-- __APP_on_message_recv
-- protobuf message 数据解析完成后调用
-- @param IClient Cpp IClient类指针 用来连接，断开，发送数据等，注册函数看文本最后
-- @param msg_full_name 待发送的protobuf message的全名（package name.message）
-- @param protobuf_msg protobuf message 数据地址指针，不可直接操作，需要配合protobuf库
function __APP_on_message_recv(IClient, msg_full_name, protobuf_msg)
    -- 以下是范例
    -- 此范例展示了如何利用protobuf库来把cpp的protobuf message类指针转换成JSON格式
    -- 再通过json.lua转换成lua的table类型来使用，从而利用回包数据内容
    --
    -- 如果收到的是login服务器的回包
    if msg_full_name == "ProtoMsg.MSG_LS2CL_LOGIN_RSP" then
        local json_data = protobuf.message_jsonencode(protobuf_msg)
        local msg_rsp = json.decode(json_data)

        IClient:disconnect()
        if msg_rsp.ret_code == "eMEC_SUCCESS" then
            g_reserve_data.player_id = msg_rsp.player_id
            g_reserve_data.login_session = msg_rsp.login_session
            IClient:connect(msg_rsp.ip, msg_rsp.port, 131073, 131073, "GS")
        end
        return

        -- 如果收到的是game服务器的回包
    elseif msg_full_name == "ProtoMsg.MSG_GS2CL_LOGIN_RSP" then
        local json_data = protobuf.message_jsonencode(protobuf_msg)
        local msg_rsp = json.decode(json_data)

        -- 如果回包不是成功则断开连接
        if msg_rsp.ret_code ~= "eMEC_SUCCESS" then
            IClient:disconnect()
        end
        return

        -- 发送心跳包
    elseif msg_full_name == "ProtoMsg.MSG_CLIENT_KEEP_LIVE_REQ" then
        IClient:sendJsonMsg("ProtoMsg.MSG_CLIENT_KEEP_LIVE_RSP", "{}")
        return
    end
end

-- __APP_on_connect_btn_click
-- 当程序界面上的连接按钮被点击后调用，在此开始登入流程等操作
-- @param IClient Cpp IClient类指针 用来连接，断开，发送数据等，注册函数看文本最后
-- @param ip 界面上填写的ip地址
-- @param port 界面上填写的端口号
-- @param account 界面上填写的账户ID
function __APP_on_connect_btn_click(IClient, ip, port, account, optional)
    -- 以下是范例
    g_reserve_data.account = account
    -- 连接login服务器
    IClient:connect(ip, port, 131073, 131073, "LS")
end

-- __APP_on_client_connected
-- 当socket链路连接成功后被调用
-- @param IClient Cpp IClient类指针 用来连接，断开，发送数据等，注册函数看文本最后
-- @param tag IClient 的connect函数中传入的自定义tag
function __APP_on_client_connected(IClient, tag)
    -- 以下是范例
    -- 如果连接的是tag==“LS”的login服务器
    if tag == "LS" then
        local register_req = {}
        register_req.info = {}
        register_req.info.server_type = 1
        IClient:sendJsonMsg("ProtoMsg.MSG_SERVER_REGIST_REQ", json.encode(register_req))

        local login_req = {}
        login_req.player_id = 0
        login_req.account = g_reserve_data.account
        IClient:sendJsonMsg("ProtoMsg.MSG_CL2LS_LOGIN_REQ", json.encode(login_req))

        -- 如果连接的是tag==“GS”的game服务器
    elseif tag == "GS" then
        local register_req = {}
        register_req.info = {}
        register_req.info.server_type = 1
        IClient:sendJsonMsg("ProtoMsg.MSG_SERVER_REGIST_REQ", json.encode(register_req))

        local login_req = {}
        login_req.player_id = g_reserve_data.player_id
        login_req.account = g_reserve_data.account
        login_req.login_session = g_reserve_data.login_session
        IClient:sendJsonMsg("ProtoMsg.MSG_CL2GS_LOGIN_REQ", json.encode(login_req))
    else
        --错误显示
    end
end

-- __APP_on_client_disconnected
-- 当socket链路断开后后被调用
-- @param IClient Cpp IClient类指针 用来连接，断开，发送数据等，注册函数看文本最后
function __APP_on_client_disconnected(IClient)
end

-- __APP_on_timer
-- 当定时器到点后被调用
-- @param timer_id
function __APP_on_timer(timer_id)
end

------------------------------ 以下为应用程序开放给lua脚本的接口
--
-- 用来获取socket缓冲区数据的类
-- class ISocketReader
-- {
-- public:
--     virtual ~ISocketReader() = default;
--     /**
--      * 这个函数用来从socket传冲区中读取uint8类型的数据
--      * @param offset 指定偏移的字节数. 范围必须满足 0 <= offset <= buf.size - 1.
--      * @returns 返回uint8类型数据
--      */
--     virtual uint8_t readUint8(size_t offset) = 0;
--     /**
--      * 这个函数用来从socket传冲区中读取int8类型的数据
--      * @param offset 指定偏移的字节数. 范围必须满足 0 <= offset <= buf.size - 1.
--      * @returns 返回int8类型数据
--      */
--     virtual int8_t readInt8(size_t offset) = 0;
--     /**
--      * 这个函数用来从socket传冲区中读取uint16类型的数据
--      * @param offset 指定偏移的字节数. 范围必须满足 0 <= offset <= buf.size - 1.
--      * @returns 返回uint16类型数据
--      */
--     virtual uint16_t readUint16(size_t offset) = 0;
--     /**
--      * 这个函数用来从socket传冲区中读取int16类型的数据
--      * @param offset 指定偏移的字节数. 范围必须满足 0 <= offset <= buf.size - 1.
--      * @returns 返回int16类型数据
--      */
--     virtual int16_t readInt16(size_t offset) = 0;
--     /**
--      * 这个函数用来从socket传冲区中读取uint32类型的数据
--      * @param offset 指定偏移的字节数. 范围必须满足 0 <= offset <= buf.size - 1.
--      * @returns 返回uint32类型数据
--      */
--     virtual uint32_t readUint32(size_t offset) = 0;
--     /**
--      * 这个函数用来从socket传冲区中读取int32类型的数据
--      * @param offset 指定偏移的字节数. 范围必须满足 0 <= offset <= buf.size - 1.
--      * @returns 返回int32类型数据
--      */
--     virtual int32_t readInt32(size_t offset) = 0;
--     /**
--      * 这个函数用来绑定protobuf message消息协议的数据给应用程序来解析
--      * @param msgFullName protobuf message 的全名（e.g: package_name.message_name)
--      * @param offset 相对于缓冲区起始点偏移的字节数
--      * @param size protobuf message消息包的大小（非网络数据包大小）
--      */
--     virtual void bindMessage(const char* msgFullName, size_t offset, size_t size) = 0;
-- };
--
-- 用来写入socket缓冲区的类
-- class ISocketWriter
-- {
-- public:
--     virtual ~ISocketWriter() = default;
--     /**
--      * 这个函数用来写入uint8类型的数据到socket缓存区
--      * @param offset 指定偏移字节数来写入
--      */
--     virtual void writeUint8(uint8_t value) = 0;
--     /**
--      * 这个函数用来写入int8类型的数据到socket缓存区
--      * @param offset 指定偏移字节数来写入
--      */
--     virtual void writeInt8(int8_t value) = 0;
--     /**
--      * 这个函数用来写入uint16类型的数据到socket缓存区
--      * @param offset 指定偏移字节数来写入
--      */
--     virtual void writeUint16(uint16_t value) = 0;
--     /**
--      * 这个函数用来写入int16类型的数据到socket缓存区
--      * @param offset 指定偏移字节数来写入
--      */
--     virtual void writeInt16(int16_t value) = 0;
--     /**
--      * 这个函数用来写入uint32类型的数据到socket缓存区
--      * @param offset 指定偏移字节数来写入
--      */
--     virtual void writeUint32(uint32_t value) = 0;
--     /**
--      * 这个函数用来写入int32类型的数据到socket缓存区
--      * @param offset 指定偏移字节数来写入
--      */
--     virtual void writeInt32(int32_t value) = 0;
--     /**
--      * 这个函数用来写入二进制数据到socket缓冲区
--      * @param pData 二进制数据指针地址
--      * @param size  二进制数据大小
--      */
--     virtual void writeBinary(void* pData, size_t size) = 0;
-- };
--
-- Proto管理类，由它来加载proto文件，可以获取google::protobuf::DescriptorPool*来使用protobuf的lua接口
-- class IProtoManager
-- {
-- public:
--     virtual ~IProtoManager() = default;
--     /**
--      * 这个函数用来添加protobuf消息到应用程序左侧的消息协议列表
--      * @param msgType 消息协议类型（ID）如果没有可填0
--      * @param msgFullName protobuf message的全名
--      * @param msgName protobuf message的名字
--      */
--     virtual void addProtoMessage(int msgType, const char* msgFullName, const char* msgName) = 0;
--     /**
--      * 这个函数用来获取protobuf的DescriptorPool指针，用来给lua的protobuf类操作
--      * @returns 返回 google::protobuf::DescriptorPool*
--      */
--     virtual void* getDescriptorPool() = 0;
-- };
--
--
-- 客户端类用来连接，断开和发送消息用
-- class IClient 
-- {
-- public:
--     virtual ~IClient() = default;
--     /**
--      * 这个函数用来连接远端服务器
--      * @param ip 远端的 ip4 地址.
--      * @param port 远端的端口号.
--      * @param recv socket 接收缓冲区的最大大小
--      * @param send socket 发送缓冲区的最大大小
--      * @param tag 这个会原封不动的传递给 __APP_on_message_recv 函数
--      */
--     virtual void connect(const char* ip, Port port, BuffSize recv, BuffSize send, const char* tag) = 0;
--     /**
--      * 看函数名就知道是啥意思了把
--      */
--     virtual void disconnect() = 0;
--     virtual bool isConnected() = 0;
--     /**
--      * 这个函数用来发送protobuf message数据包
--      * @param msgFullName protobuf message 全名.
--      * @param jsonData JSON格式的protobuf message的数据
--      */
--     virtual void sendJsonMsg(const char* msgFullName, const char* jsonData) = 0;
-- };
--
-- class IMainApp
-- {
-- public:
--     virtual ~IMainApp() = default;
--     /**
--      * This function is used to add timer
--      * @param interval milliseconds 
--      * @returns return timer id 
--      */
--     virtual int addTimer(int interval) = 0;
--     /**
--      * This function is used to delete timer
--      * @param timerId 
--      */
--     virtual void deleteTimer(int timerId) = 0;
--
--     virtual IClient* getClient() = 0;
-- };

------------------------------ 以下为应用程序开放给lua的protobuf的接口，使用方法可参考官方protobuf库
-- // importer
-- void* protobuf_createimporter(const char* path);
-- void protobuf_freeimporter(void* c);
-- void* protobuf_importer_import(void* c, const char* path);
-- void* protobuf_importer_getpool(void* c);
-- 
-- // descripor pool
-- void* protobuf_descriptorpool_findmessagetypebyname(void* pool, const char* name);
-- void* protobuf_descriptorpool_findfieldbyname(void* pool, const char* name);
-- void* protobuf_descriptorpool_findoneofbyname(void* pool, const char* name);
-- void* protobuf_descriptorpool_findenumtypebyname(void* pool, const char* name);
-- void* protobuf_descriptorpool_findenumvaluebyname(void* pool, const char* name);
-- 
-- // file descriptor
-- int protobuf_filedescriptor_messagetypecount(void* descriptor);
-- void* protobuf_filedescriptor_messagetype(void* descriptor, int index);
-- int protobuf_filedescriptor_enumtypecount(void* descriptor);
-- void* protobuf_filedescriptor_enumtype(void* descriptor, int index);
-- 
-- // message descriptor
-- const char* protobuf_messagedescriptor_name(void* descriptor);
-- const char* protobuf_messagedescriptor_fullname(void* descriptor);
-- int protobuf_messagedescriptor_fieldcount(void* descriptor);
-- void* protobuf_messagedescriptor_field(void* descriptor, int index);
-- void* protobuf_messagedescriptor_findfieldbyname(void* descriptor, const char* name);
-- void* protobuf_messagedescriptor_findfieldbynumber(void* descriptor, int number);
-- 
-- // enum descriptor
-- const char* protobuf_enumdescriptor_name(void* descriptor);
-- const char* protobuf_enumdescriptor_fullname(void* descriptor);
-- int protobuf_enumdescriptor_valuecount(void* descriptor);
-- void* protobuf_enumdescriptor_value(void* descriptor, int index);
-- void* protobuf_enumdescriptor_findvaluebyname(void* descriptor, const char* name);
-- void* protobuf_enumdescriptor_findvaluebynumber(void* descriptor, int number);
-- 
-- // enum value descriptor
-- const char* protobuf_enumvaluedescriptor_name(void* descriptor);
-- int protobuf_enumvaluedescriptor_number(void* descriptor);
-- 
-- // field descriptor
-- int protobuf_fielddescriptor_cpptype(void* descriptor);
-- const char* protobuf_fielddescriptor_name(void* descriptor);
-- const char* protobuf_fielddescriptor_cpptypename(void* descriptor);
-- int protobuf_fielddescriptor_ismap(void* descriptor);
-- int protobuf_fielddescriptor_isrepeated(void* descriptor);
-- void* protobuf_fielddescriptor_messagetype(void* descriptor);
-- void* protobuf_fielddescriptor_enumtype(void* descriptor);
-- 
-- // factory
-- void* protobuf_createfactory();
-- void protobuf_freefactory(void* c);
-- void* protobuf_factory_createmessage(void* c, void* message_descriptor);
-- 
-- // message
-- // @param *m 为message的指针的指针
-- void protobuf_freemessage(void* m);
-- void* protobuf_clonemessage(void* m);
-- void* protobuf_message_getdescriptor(void* m);
-- void* protobuf_message_getreflection(void* m);
-- int protobuf_message_getbytesize(void* m);
-- int protobuf_message_serializetoarray(void* m, void* buffer, int size);
-- int protobuf_message_parsefromarray(void* m, void* buffer, int size);
-- const char* protobuf_message_jsonencode(void* m);
-- const char* protobuf_message_jsondecode(void* m, const char* json);
-- 
-- // reflection
-- // @param *r 为reflection的指针
-- // @param *m 为message的指针
-- // @param *field 为field descriptor 的指针
-- void* protobuf_reflection_getmessage(void* r, void* m, void* field);
-- int protobuf_reflection_getbool(void* r, void* m, void* field);
-- int protobuf_reflection_getint32(void* r, void* m, void* field);
-- unsigned int protobuf_reflection_getuint32(void* r, void* m, void* field);
-- long long protobuf_reflection_getint64(void* r, void* m, void* field);
-- unsigned long long protobuf_reflection_getuint64(void* r, void* m, void* field);
-- double protobuf_reflection_getdouble(void* r, void* m, void* field);
-- float protobuf_reflection_getfloat(void* r, void* m, void* field);
-- int protobuf_reflection_getenumvalue(void* r, void* m, void* field);
-- const char* protobuf_reflection_getstring(void* r, void* m, void* field);
-- 
-- void protobuf_reflection_setbool(void* r, void* m, void* field, int value);
-- void protobuf_reflection_setint32(void* r, void* m, void* field, int value);
-- void protobuf_reflection_setuint32(void* r, void* m, void* field, unsigned int value);
-- void protobuf_reflection_setint64(void* r, void* m, void* field, long long value);
-- void protobuf_reflection_setuint64(void* r, void* m, void* field, unsigned long long value);
-- void protobuf_reflection_setdouble(void* r, void* m, void* field, double value);
-- void protobuf_reflection_setfloat(void* r, void* m, void* field, float value);
-- void protobuf_reflection_setenumvalue(void* r, void* m, void* field, int value);
-- void protobuf_reflection_setstring(void* r, void* m, void* field, const char* value);
-- 
-- void protobuf_reflection_clearfield(void* r, void* m, void* field);
-- 
-- int protobuf_reflection_getrepeatedmessagecount(void* r, void* m, void* field);
-- int protobuf_reflection_getrepeatedboolcount(void* r, void* m, void* field);
-- int protobuf_reflection_getrepeatedint32count(void* r, void* m, void* field);
-- int protobuf_reflection_getrepeateduint32count(void* r, void* m, void* field);
-- int protobuf_reflection_getrepeatedint64count(void* r, void* m, void* field);
-- int protobuf_reflection_getrepeateduint64count(void* r, void* m, void* field);
-- int protobuf_reflection_getrepeateddoublecount(void* r, void* m, void* field);
-- int protobuf_reflection_getrepeatedfloatcount(void* r, void* m, void* field);
-- int protobuf_reflection_getrepeatedstringcount(void* r, void* m, void* field);
-- 
-- void* protobuf_reflection_getrepeatedmessage(void* r, void* m, void* field, int index);
-- int protobuf_reflection_getrepeatedbool(void* r, void* m, void* field, int index);
-- int protobuf_reflection_getrepeatedint32(void* r, void* m, void* field, int index);
-- unsigned int protobuf_reflection_getrepeateduint32(void* r, void* m, void* field, int index);
-- long long protobuf_reflection_getrepeatedint64(void* r, void* m, void* field, int index);
-- unsigned long long protobuf_reflection_getrepeateduint64(void* r, void* m, void* field, int index);
-- double protobuf_reflection_getrepeateddouble(void* r, void* m, void* field, int index);
-- float protobuf_reflection_getrepeatedfloat(void* r, void* m, void* field, int index);
-- const char* protobuf_reflection_getrepeatedstring(void* r, void* m, void* field, int index);
-- 
-- void protobuf_reflection_setrepeatedbool(void* r, void* m, void* field, int index, int value);
-- void protobuf_reflection_setrepeatedint32(void* r, void* m, void* field, int index, int value);
-- void protobuf_reflection_setrepeateduint32(void* r, void* m, void* field, int index, unsigned int value);
-- void protobuf_reflection_setrepeatedint64(void* r, void* m, void* field, int index, long long value);
-- void protobuf_reflection_setrepeateduint64(void* r, void* m, void* field, int index, unsigned long long value);
-- void protobuf_reflection_setrepeateddouble(void* r, void* m, void* field, int index, double value);
-- void protobuf_reflection_setrepeatedfloat(void* r, void* m, void* field, int index, float value);
-- void protobuf_reflection_setrepeatedstring(void* r, void* m, void* field, int index, const char* value);
-- 
-- void* protobuf_reflection_insertrepeatedmessage(void* r, void* m, void* field, int index);
-- void protobuf_reflection_insertrepeatedbool(void* r, void* m, void* field, int index, int value);
-- void protobuf_reflection_insertrepeatedint32(void* r, void* m, void* field, int index, int value);
-- void protobuf_reflection_insertrepeateduint32(void* r, void* m, void* field, int index, unsigned int value);
-- void protobuf_reflection_insertrepeatedint64(void* r, void* m, void* field, int index, long long value);
-- void protobuf_reflection_insertrepeateduint64(void* r, void* m, void* field, int index, unsigned long long value);
-- void protobuf_reflection_insertrepeateddouble(void* r, void* m, void* field, int index, double value);
-- void protobuf_reflection_insertrepeatedfloat(void* r, void* m, void* field, int index, float value);
-- void protobuf_reflection_insertrepeatedstring(void* r, void* m, void* field, int index, const char* value);
-- 
-- void protobuf_reflection_removerepeatedmessage(void* r, void* m, void* field, int index);
-- void protobuf_reflection_removerepeatedbool(void* r, void* m, void* field, int index);
-- void protobuf_reflection_removerepeatedint32(void* r, void* m, void* field, int index);
-- void protobuf_reflection_removerepeateduint32(void* r, void* m, void* field, int index);
-- void protobuf_reflection_removerepeatedint64(void* r, void* m, void* field, int index);
-- void protobuf_reflection_removerepeateduint64(void* r, void* m, void* field, int index);
-- void protobuf_reflection_removerepeateddouble(void* r, void* m, void* field, int index);
-- void protobuf_reflection_removerepeatedfloat(void* r, void* m, void* field, int index);
-- void protobuf_reflection_removerepeatedstring(void* r, void* m, void* field, int index);

