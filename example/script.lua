package.path = "./script/?.lua"

local get_full_name_by_type = {}
local get_type_by_full_name = {}

local g_reserve_data = {}

function __APP_on_proto_reload(ProtoManager)
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
            local msg_name = string.sub(field_name, 2)
            local full_name = "ProtoMsg." .. msg_name
            local msg_descriptor = protobuf.descriptorpool_findmessagetypebyname(pool, full_name)
            if msg_descriptor ~= nil then
                get_full_name_by_type[field_number] = full_name
                get_type_by_full_name[full_name] = field_number
                if string.find(msg_name, "_CL2") ~= nil 
                    or string.find(msg_name, "2CL_") then
                    ProtoManager:addProtoMessage(field_number, full_name, msg_name)
                end
            end
        end
    end
    return
end

function __APP_on_read_socket_buffer(SocketReader, buff_size)
    local msg_head_size = 8

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
    local msg_size = packet_size - 8
    SocketReader:bindMessage(msg_full_name, 0 + msg_head_size, msg_size)
    return packet_size
end

function __APP_on_write_socket_buffer(SocketWriter, msg_full_name, protobuf_data, protobuf_size)
    local msg_type = get_type_by_full_name[msg_full_name]
    if msg_type == nil then
        return
    end
    SocketWriter:writeUint16(protobuf_size + 8)
    SocketWriter:writeUint16(msg_type)
    SocketWriter:writeUint16(1)
    SocketWriter:writeUint16(1)
    SocketWriter:writeBinary(protobuf_data, protobuf_size)
end

function __APP_on_message_recv(Client, msg_full_name, protobuf_msg)
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
    elseif msg_full_name == "ProtoMsg.MSG_GS2CL_LOGIN_RSP" then
        local json_data = protobuf.message_jsonencode(protobuf_msg)
        local msg_rsp = json.decode(json_data)

        if msg_rsp.ret_code ~= "eMEC_SUCCESS" then
            Client:disconnect()
            return
        end

        Client:sendJsonMsg("ProtoMsg.MSG_CL2GS_CHAT_LOGIN_REQ", "{}")
        return
    elseif msg_full_name == "ProtoMsg.MSG_GS2CL_LOGIN_DATA_READY_NTF" then
        return
    elseif msg_full_name == "ProtoMsg.MSG_CLIENT_KEEP_LIVE_REQ" then
        Client:sendJsonMsg("ProtoMsg.MSG_CLIENT_KEEP_LIVE_RSP", "{}")
        return
    elseif msg_full_name == "ProtoMsg.MSG_GS2CL_CHAT_LOGIN_RSP" then
        local json_data = protobuf.message_jsonencode(protobuf_msg)
        local msg_rsp = json.decode(json_data)

        if msg_rsp.ret_code == "eCE_OK" then
            g_reserve_data.login_code = msg_rsp.login_code
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

function __APP_on_connect_btn_click(ip, port, account, optional)
    g_reserve_data.account = account

    local Client = App:createClient("LS")
    if Client ~= nil then
        Client:connect(ip, port, 131073, 131073)
    end
end


function __APP_on_client_connected(Client)
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

function __APP_on_client_disconnected(name)
end

function __APP_on_timer(timer_id)
end
