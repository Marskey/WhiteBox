package.path = "./script/?.lua"

local get_full_name_by_type = {}
local get_type_by_full_name = {}

local g_reserve_data = {}

function __APP_on_proto_reload(ProtoManager)
    local pool = ProtoManager:getDescriptorPool()
    if pool == nil then
        return
    end
    local enum_descriptor = protobuf.descriptorpool_findenumtypebyname(pool, "msgtype.MsgType")
    if enum_descriptor == nil then
        return
    end
    local count = protobuf.enumdescriptor_valuecount(enum_descriptor)
    for i = 0, count - 1, 1 do
        local enumvalue_descriptor = protobuf.enumdescriptor_value(enum_descriptor, i)
        if enumvalue_descriptor ~= nil then
            local field_name = protobuf.enumvaluedescriptor_name(enumvalue_descriptor)
            local field_number = protobuf.enumvaluedescriptor_number(enumvalue_descriptor)
            local msg_name = string.sub(field_name, 2)
            local full_name = "protomsg." .. msg_name
            if string.find(full_name, "2CL") ~= nil or string.find(full_name, "CL2") ~= nil then
                local msg_descriptor = protobuf.descriptorpool_findmessagetypebyname(pool, full_name)
                if msg_descriptor ~= nil then
                    get_full_name_by_type[field_number] = full_name
                    get_type_by_full_name[full_name] = field_number
                    ProtoManager:addProtoMessage(field_number, full_name, msg_name)
                end
            end
        end
    end
    return
end

function __APP_on_read_socket_buffer(SocketReader, buff_size)
    local msg_head_size = 4

    if buff_size < msg_head_size then
        return 0
    end

    local packet_size = SocketReader:readUint16(0)
    if packet_size <= 0 then
        return 0
    end

    local msg_type = SocketReader:readUint16(2)
    if msg_type == 0 then
        return 0
    end

    local msg_full_name = get_full_name_by_type[msg_type]
    local msg_size = packet_size - msg_head_size
    SocketReader:bindMessage(msg_full_name, 0 + msg_head_size, msg_size)
    return packet_size
end

function __APP_on_write_socket_buffer(SocketWrite, msg_full_name, protobuf_data, protobuf_size)
    local msg_type = get_type_by_full_name[msg_full_name]
    if msg_type == nil then
        return
    end
    SocketWrite:writeUint16(protobuf_size + 4)
    SocketWrite:writeUint16(msg_type)
    SocketWrite:writeBinary(protobuf_data, protobuf_size)
end

function __APP_on_message_recv(Client, msg_full_name, protobuf_msg)
    if msg_full_name == "protomsg.MsgLS2CLLoginReply" then
        local json_data = protobuf.message_jsonencode(protobuf_msg)
        local msg_rsp = json.decode(json_data)
        Client:disconnect()
        if msg_rsp.error_code == 0 then
            g_reserve_data.player_id = msg_rsp.player_id
            g_reserve_data.login_session = msg_rsp.login_session
            local GSClient = App:createClient("Game Server")
            if GSClient ~= nil then
                GSClient:connect(msg_rsp.net_address.ip, msg_rsp.net_address.port, 131073, 131073)
            end
        end
        return
    elseif msg_full_name == "protomsg.MsgGS2CLLoginReply" then
        local json_data = protobuf.message_jsonencode(protobuf_msg)
        local msg_rsp = json.decode(json_data)

        if msg_rsp.error_code ~= 0 then
            Client:disconnect()
        end

        local enter_req = {}
        enter_req.mac = "marskey"
        Client:sendJsonMsg("protomsg.MsgCL2GSEnterGameRequest", json.encode(enter_req))
        return
    end
end

function __APP_on_connect_btn_click(ip, port, account, optional)
    g_reserve_data.account = account
    local Client = App:createClient("Login Server")
    if Client ~= nil then
        Client:connect(ip, port, 131073, 131073)
    end
end

local g_timer = 0
function __APP_on_client_connected(Client)
    if Client:getName() == "Login Server" then
        local login_req = {}
        login_req.player_id = 0
        login_req.account = g_reserve_data.account
        login_req.area_id = 1
        Client:sendJsonMsg("protomsg.MsgCL2LSLoginRequest", json.encode(login_req))

    elseif Client:getName() == "Game Server" then
        local login_req = {}
        login_req.player_id = g_reserve_data.player_id
        login_req.account = g_reserve_data.account
        login_req.login_session = g_reserve_data.login_session
        Client:sendJsonMsg("protomsg.MsgCL2GSLoginRequest", json.encode(login_req))

        g_timer = App:addTimer(10 * 60 * 1000)
    else
        --错误显示
    end
end

function __APP_on_client_disconnected(name)
    App:deleteTimer(g_timer)
end

function __APP_on_timer(timer_id)
    if timer_id == g_timer then
        local Client = App:getClient("Game Server")
        if Client:isConnected() ~= false then
            Client:sendJsonMsg("protomsg.MsgCL2GSKeepLiveRequest", "{}")
        end
    end
end
