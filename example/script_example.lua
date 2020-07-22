package.path = package.path..";./script/?.lua"
local get_full_name_by_type = {}
local get_type_by_full_name = {}
local g_reserve_data = {}
-- 定义包头大小
local msg_head_size = 6
-- __APP_on_proto_reload
-- 每当客户端加载完proto文件后被调用
-- @param ProtoMananger Cpp类的实例指针，注册函数看文本最后
function __APP_on_proto_reload(ProtoManager)
    -- 以下是范例
    local pool = ProtoManager:getDescriptorPool()
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

            get_full_name_by_type[field_number] = full_name
            get_type_by_full_name[full_name] = field_number

            -- message type 定义在了EMsgTypes所以这个field_number就是相当于消息协议中的message id
            -- 如果没有 message type 可以填0（不会吧，不会真的有人没有定义message type吧，不会吧）
            if string.find(full_name, "2CL") ~= nil or string.find(full_name, "CL2") ~= nil then
                -- 将消息名中含有2CL或者CL2的消息协议添加到可发送列表
                ProtoManager:addProtoMessage(field_number, full_name, msg_name)
            end
        end
    end
    return
end
-- __APP_on_read_socket_buffer
-- 每当客户端接收到网络数据包后被调用, 需要在该函数中对数据块进行拆包操作（解决粘包问题）
-- @param SocketReader Cpp ISocketReader类指针 网络数据包块，注册函数看文本最后
-- @param buff_size 网络数据包块的大小
-- @returns packet_size 返回拆包后一个包的大小，如果返回0则会继续等待数据包到来后被调用
function __APP_on_read_socket_buffer(SocketReader, buff_size)
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

    local packet_size = SocketReader:readUint16(0)
    if packet_size <= 0 then
        return 0
    end

    local msg_type = SocketReader:readUint16(2)
    if msg_type == 0 then
        return 0
    end

    local flag = SocketReader:readUint8(4)

    local opt = SocketReader:readUint8(5)

    local msg_full_name = get_full_name_by_type[msg_type]
    local msg_size = packet_size - msg_head_size

    -- 用bindMessage来告诉应用程序需要解析的消息结构，因为消息数据在包头之后
    -- ，所以用getDataPtr获取消息体数据起始指针
    local msg_data_ptr = SocketReader:getDataPtr(msg_head_size)
    SocketReader:bindMessage(msg_full_name, msg_data_ptr, msg_size)
    return packet_size
end
-- __APP_on_write_socket_buffer
-- 每当客户端发送数据包前被调用，需要在此实现对数据包头的添加
-- @param ISocketWriter Cpp ISocketWriter类指针 待发送的网络数据包块，注册函数看文本最后
-- @param msg_full_name 待发送的protobuf message的全名（package name.message）
-- @param protobuf_data protobuf message cpp数据地址指针
-- @param protobuf_size protobuf message 数据大小
function __APP_on_write_socket_buffer(SocketWriter, msg_full_name, protobuf_data, protobuf_size)
    -- 以下是范例
    -- 此范例展示了如何向socket缓冲区写入包数据
    local msg_type = get_type_by_full_name[msg_full_name]
    if msg_type == nil then
        return
    end
    SocketWriter:writeUint16(protobuf_size + msg_head_size)
    SocketWriter:writeUint16(msg_type)
    SocketWriter:writeUint8(0)
    SocketWriter:writeUint8(1)
    SocketWriter:writeBinary(protobuf_data, protobuf_size)
end
-- __APP_on_message_recv
-- protobuf message 数据解析完成后调用
-- @param Client Cpp IClient类指针 用来连接，断开，发送数据等，注册函数看文本最后
-- @param msg_full_name 待发送的protobuf message的全名（package name.message）
-- @param protobuf_msg protobuf message 数据地址指针，不可直接操作，需要配合protobuf库
function __APP_on_message_recv(Client, msg_full_name, protobuf_msg)
    -- 以下是范例
    -- 此范例展示了如何利用protobuf库来把cpp的protobuf message类指针转换成JSON格式
    -- 再通过json.lua转换成lua的table类型来使用，从而利用回包数据内容
    --
    -- 如果收到的是login服务器的回包
    if msg_full_name == "ProtoMsg.MSG_LS2CL_LOGIN_RSP" then
        local json_data = protobuf.message_jsonencode(protobuf_msg)
        local msg_rsp = json.decode(json_data)

        Client:disconnect()
        if msg_rsp.ret_code == "eMEC_SUCCESS" then
            g_reserve_data.player_id = msg_rsp.player_id
            g_reserve_data.login_session = msg_rsp.login_session

            local GSClient = App:createClient("GS")
            if GSClient ~= nil then
                GSClient:connect(msg_rsp.net_addr.ip, msg_rsp.net_addr.port, 131073, 131073)
            end
        end
        return

        -- 如果收到的是game服务器的回包
    elseif msg_full_name == "ProtoMsg.MSG_GS2CL_LOGIN_RSP" then
        local json_data = protobuf.message_jsonencode(protobuf_msg)
        local msg_rsp = json.decode(json_data)

        -- 如果回包不是成功则断开连接
        if msg_rsp.ret_code ~= "eMEC_SUCCESS" then
            Client:disconnect()
        end
        return

        -- 向GS获取ChatServer的登入相关信息
        Client:sendJsonMsg("ProtoMsg.MSG_CL2GS_CHAT_LOGIN_REQ", "{}")

        -- 发送心跳包
    elseif msg_full_name == "ProtoMsg.MSG_CLIENT_KEEP_LIVE_REQ" then
        Client:sendJsonMsg("ProtoMsg.MSG_CLIENT_KEEP_LIVE_RSP", "{}")
        return

        -- 如果收到了获取chatServer登入相关信息
    elseif msg_full_name == "ProtoMsg.MSG_GS2CL_CHAT_LOGIN_RSP" then
        local json_data = protobuf.message_jsonencode(protobuf_msg)
        local msg_rsp = json.decode(json_data)

        if msg_rsp.ret_code == "eCE_OK" then
            g_reserve_data.login_code = msg_rsp.login_code

            -- 获取一个新的名字叫CH的客户端
            local CHClient = App:createClient("CH")
            if CHClient ~= nil then
                CHClient:connect(msg_rsp.ip, msg_rsp.port, 131073, 131073)
            end
        end
        return
    elseif msg_full_name == "ProtoMsg.MSG_CH2CL_CHAT_LOGIN_RSP" then
        local json_data = protobuf.message_jsonencode(protobuf_msg)
        local msg_rsp = json.decode(json_data)
        if msg_rsp.ret_code ~= "eCE_OK" then
            Client:disconnect()
        end
        return
    end
end
-- __APP_on_connect_btn_click
-- 当程序界面上的连接按钮被点击后调用，在此开始登入流程等操作
-- @param ip 界面上填写的ip地址
-- @param port 界面上填写的端口号
-- @param account 界面上填写的账户ID
-- @param optional 界面上填写可选参数（自行解析）
function __APP_on_connect_btn_click(ip, port, account, optional)
    -- 以下是范例
    g_reserve_data.account = account
    -- 连接login服务器
    -- 创建一个名字叫LS的客户端
    local Client = App:createClient("LS")
    if Client ~= nil then
        Client:connect(ip, port, 131073, 131073)
    end
end
-- __APP_on_client_connected
-- 当socket链路连接成功后被调用
-- @param Client Cpp IClient类指针 用来连接，断开，发送数据等，注册函数看文本最后
function __APP_on_client_connected(Client)
    -- 以下是范例
    -- 如果连接的是名字叫“LS”的客户端对象
    if Client:getName() == "LS" then
        local register_req = {}
        register_req.info = {}
        register_req.info.server_type = 1
        Client:sendJsonMsg("ProtoMsg.MSG_SERVER_REGIST_REQ", json.encode(register_req))

        local login_req = {}
        login_req.player_id = 0
        login_req.account = g_reserve_data.account
        login_req.login_for_dev = "true"
        Client:sendJsonMsg("ProtoMsg.MSG_CL2LS_LOGIN_REQ", json.encode(login_req))

    -- 如果连接的是名字叫“GS”的客户端对象
    elseif Client:getName() == "GS" then
        local register_req = {}
        register_req.info = {}
        register_req.info.server_type = 1
        Client:sendJsonMsg("ProtoMsg.MSG_SERVER_REGIST_REQ", json.encode(register_req))

        local login_req = {}
        login_req.player_id = g_reserve_data.player_id
        login_req.account = g_reserve_data.account
        login_req.login_session = g_reserve_data.login_session
        Client:sendJsonMsg("ProtoMsg.MSG_CL2GS_LOGIN_REQ", json.encode(login_req))

    -- 如果连接的是名字叫“CH”的客户端对象
    elseif Client:getName() == "CH" then
        local register_req = {}
        register_req.info = {}
        register_req.info.server_type = 1
        Client:sendJsonMsg("ProtoMsg.MSG_SERVER_REGIST_REQ", json.encode(register_req))

        local login_req = {}
        login_req.player_id = g_reserve_data.player_id
        login_req.login_code = g_reserve_data.login_code
        Client:sendJsonMsg("ProtoMsg.MSG_CL2CH_CHAT_LOGIN_REQ", json.encode(login_req))
    else
        --错误显示
    end
end
-- __APP_on_client_disconnected
-- 当socket链路断开后后被调用
-- @param name 断开连接的客户端名字
function __APP_on_client_disconnected(name)
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
--      * 这个函数使用来socket传冲区中消息体数据指针
--      * @param offset 指定偏移的字节数. 范围必须满足 0 <= offset <= buf.size - 1.
--      * @returns 返回void*
--      */
--     virtual void* getDataPtr(size_t offset) = 0;
--     /**
--      * 这个函数用来绑定protobuf message消息协议的数据给应用程序来解析
--      * @param msgFullName protobuf message 的全名（e.g: package_name.message_name)
--      * @param void* protobuf message消息包数据的起始地址
--      * @param size protobuf message消息包的大小（非网络数据包大小）
--      */
--     virtual void bindMessage(const char* msgFullName, void* pData, size_t size) = 0;
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
--      */
--     virtual void connect(const char* ip, Port port, BuffSize recv, BuffSize send) = 0;
--     /**
--      * 看函数名就知道是啥意思了把
--      */
--     virtual void disconnect() = 0;
--     virtual bool isConnected() = 0;
--     /**
--       * 这个函数用来获取客户端名字
--       */
--      virtual const char* getName() = 0;-
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
--      * 这个函数用来添加定时器
--      * @param 间隔触发的毫秒时间
--      * @returns 返回定时器id timer id 
--      */
--     virtual int addTimer(int interval) = 0;
--     /**
--      * 这个函数用来删除定时器
--      * @param timerId 定时器id
--      */
--     virtual void deleteTimer(int timerId) = 0;
--
--      /*
--       * 这个函数用来创建一个客户端，连接成功的客户端将会在发送按钮边上的下拉列表中显示
--       * @param name 客户端名字（不能为空）
--       */
--      virtual IClient* createClient(const char* name) = 0;
--      /**
--       * 这个函数用来获取一个指定名字的客户端
--       * @param name 客户端名字（不能为空）
--       */
--      virtual IClient* getClient(const char* name) = 0;
-- };

------------------------------ 以下为应用程序开放给lua的protobuf的接口，使用方法可参考官方protobuf库
-- // importer
-- void* createimporter(const char* path);
-- void freeimporter(void* c);
-- void* importer_import(void* c, const char* path);
-- void* importer_getpool(void* c);
-- 
-- // descripor pool
-- void* descriptorpool_findfilebyname(void* pool, const char* name);
-- void* descriptorpool_findmessagetypebyname(void* pool, const char* name);
-- void* descriptorpool_findfieldbyname(void* pool, const char* name);
-- void* descriptorpool_findoneofbyname(void* pool, const char* name);
-- void* descriptorpool_findenumtypebyname(void* pool, const char* name);
-- void* descriptorpool_findenumvaluebyname(void* pool, const char* name);
-- 
-- // file descriptor
-- int filedescriptor_messagetypecount(void* descriptor);
-- void* filedescriptor_messagetype(void* descriptor, int index);
-- int filedescriptor_enumtypecount(void* descriptor);
-- void* filedescriptor_enumtype(void* descriptor, int index);
-- 
-- // message descriptor
-- const char* messagedescriptor_name(void* descriptor);
-- const char* messagedescriptor_fullname(void* descriptor);
-- int messagedescriptor_fieldcount(void* descriptor);
-- void* messagedescriptor_field(void* descriptor, int index);
-- void* messagedescriptor_findfieldbyname(void* descriptor, const char* name);
-- void* messagedescriptor_findfieldbynumber(void* descriptor, int number);
-- 
-- // enum descriptor
-- const char* enumdescriptor_name(void* descriptor);
-- const char* enumdescriptor_fullname(void* descriptor);
-- int enumdescriptor_valuecount(void* descriptor);
-- void* enumdescriptor_value(void* descriptor, int index);
-- void* enumdescriptor_findvaluebyname(void* descriptor, const char* name);
-- void* enumdescriptor_findvaluebynumber(void* descriptor, int number);
-- 
-- // enum value descriptor
-- const char* enumvaluedescriptor_name(void* descriptor);
-- int enumvaluedescriptor_number(void* descriptor);
-- 
-- // field descriptor
-- int fielddescriptor_cpptype(void* descriptor);
-- const char* fielddescriptor_name(void* descriptor);
-- const char* fielddescriptor_cpptypename(void* descriptor);
-- int fielddescriptor_ismap(void* descriptor);
-- int fielddescriptor_isrepeated(void* descriptor);
-- void* fielddescriptor_messagetype(void* descriptor);
-- void* fielddescriptor_enumtype(void* descriptor);
-- 
-- // factory
-- void* createfactory();
-- void freefactory(void* c);
-- void* factory_createmessage(void* c, void* message_descriptor);
-- 
-- // message
-- // @param *m 为message的指针的指针
-- void freemessage(void* m);
-- void* clonemessage(void* m);
-- void* message_getdescriptor(void* m);
-- void* message_getreflection(void* m);
-- int message_getbytesize(void* m);
-- int message_serializetoarray(void* m, void* buffer, int size);
-- int message_parsefromarray(void* m, void* buffer, int size);
-- const char* message_jsonencode(void* m);
-- const char* message_jsondecode(void* m, const char* json);
-- 
-- // reflection
-- // @param *r 为reflection的指针
-- // @param *m 为message的指针
-- // @param *field 为field descriptor 的指针
-- void* reflection_getmessage(void* r, void* m, void* field);
-- int reflection_getbool(void* r, void* m, void* field);
-- int reflection_getint32(void* r, void* m, void* field);
-- unsigned int reflection_getuint32(void* r, void* m, void* field);
-- long long reflection_getint64(void* r, void* m, void* field);
-- unsigned long long reflection_getuint64(void* r, void* m, void* field);
-- double reflection_getdouble(void* r, void* m, void* field);
-- float reflection_getfloat(void* r, void* m, void* field);
-- int reflection_getenumvalue(void* r, void* m, void* field);
-- const char* reflection_getstring(void* r, void* m, void* field);
-- 
-- void reflection_setbool(void* r, void* m, void* field, int value);
-- void reflection_setint32(void* r, void* m, void* field, int value);
-- void reflection_setuint32(void* r, void* m, void* field, unsigned int value);
-- void reflection_setint64(void* r, void* m, void* field, long long value);
-- void reflection_setuint64(void* r, void* m, void* field, unsigned long long value);
-- void reflection_setdouble(void* r, void* m, void* field, double value);
-- void reflection_setfloat(void* r, void* m, void* field, float value);
-- void reflection_setenumvalue(void* r, void* m, void* field, int value);
-- void reflection_setstring(void* r, void* m, void* field, const char* value);
-- 
-- void reflection_clearfield(void* r, void* m, void* field);
-- 
-- int reflection_getrepeatedmessagecount(void* r, void* m, void* field);
-- int reflection_getrepeatedboolcount(void* r, void* m, void* field);
-- int reflection_getrepeatedint32count(void* r, void* m, void* field);
-- int reflection_getrepeateduint32count(void* r, void* m, void* field);
-- int reflection_getrepeatedint64count(void* r, void* m, void* field);
-- int reflection_getrepeateduint64count(void* r, void* m, void* field);
-- int reflection_getrepeateddoublecount(void* r, void* m, void* field);
-- int reflection_getrepeatedfloatcount(void* r, void* m, void* field);
-- int reflection_getrepeatedstringcount(void* r, void* m, void* field);
-- 
-- void* reflection_getrepeatedmessage(void* r, void* m, void* field, int index);
-- int reflection_getrepeatedbool(void* r, void* m, void* field, int index);
-- int reflection_getrepeatedint32(void* r, void* m, void* field, int index);
-- unsigned int reflection_getrepeateduint32(void* r, void* m, void* field, int index);
-- long long reflection_getrepeatedint64(void* r, void* m, void* field, int index);
-- unsigned long long reflection_getrepeateduint64(void* r, void* m, void* field, int index);
-- double reflection_getrepeateddouble(void* r, void* m, void* field, int index);
-- float reflection_getrepeatedfloat(void* r, void* m, void* field, int index);
-- const char* reflection_getrepeatedstring(void* r, void* m, void* field, int index);
-- 
-- void reflection_setrepeatedbool(void* r, void* m, void* field, int index, int value);
-- void reflection_setrepeatedint32(void* r, void* m, void* field, int index, int value);
-- void reflection_setrepeateduint32(void* r, void* m, void* field, int index, unsigned int value);
-- void reflection_setrepeatedint64(void* r, void* m, void* field, int index, long long value);
-- void reflection_setrepeateduint64(void* r, void* m, void* field, int index, unsigned long long value);
-- void reflection_setrepeateddouble(void* r, void* m, void* field, int index, double value);
-- void reflection_setrepeatedfloat(void* r, void* m, void* field, int index, float value);
-- void reflection_setrepeatedstring(void* r, void* m, void* field, int index, const char* value);
-- 
-- void* reflection_insertrepeatedmessage(void* r, void* m, void* field, int index);
-- void reflection_insertrepeatedbool(void* r, void* m, void* field, int index, int value);
-- void reflection_insertrepeatedint32(void* r, void* m, void* field, int index, int value);
-- void reflection_insertrepeateduint32(void* r, void* m, void* field, int index, unsigned int value);
-- void reflection_insertrepeatedint64(void* r, void* m, void* field, int index, long long value);
-- void reflection_insertrepeateduint64(void* r, void* m, void* field, int index, unsigned long long value);
-- void reflection_insertrepeateddouble(void* r, void* m, void* field, int index, double value);
-- void reflection_insertrepeatedfloat(void* r, void* m, void* field, int index, float value);
-- void reflection_insertrepeatedstring(void* r, void* m, void* field, int index, const char* value);
-- 
-- void reflection_removerepeatedmessage(void* r, void* m, void* field, int index);
-- void reflection_removerepeatedbool(void* r, void* m, void* field, int index);
-- void reflection_removerepeatedint32(void* r, void* m, void* field, int index);
-- void reflection_removerepeateduint32(void* r, void* m, void* field, int index);
-- void reflection_removerepeatedint64(void* r, void* m, void* field, int index);
-- void reflection_removerepeateduint64(void* r, void* m, void* field, int index);
-- void reflection_removerepeateddouble(void* r, void* m, void* field, int index);
-- void reflection_removerepeatedfloat(void* r, void* m, void* field, int index);
-- void reflection_removerepeatedstring(void* r, void* m, void* field, int index);

