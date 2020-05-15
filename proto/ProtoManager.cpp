#include "ProtoManager.h"
#include "google/protobuf/dynamic_message.h"
#include "libCommon/Utility/File.h"

CProtoManager::CProtoManager() {
    
}

CProtoManager::~CProtoManager() {
    
}

bool CProtoManager::importMsg(const char* path, const char* rootPath) {
    CFile protoPath(path);
    protoPath.tryRemoveCLRF();
    protoPath.tryAddTailSeparator();
    if (protoPath.getPath().empty()
        || !protoPath.folderExists()) {
        return false;
    }

    CFile protoRoot = rootPath;
    protoRoot.tryRemoveCLRF();
    protoRoot.tryAddTailSeparator();
    if (protoRoot.getPath().empty()
        || !protoRoot.folderExists()) {
        return false;
    }
    
    // 列出proto目录下的所有文件
    std::vector<std::string> fileList;
    protoPath.getDeepFileNameList(fileList);

    google::protobuf::compiler::DiskSourceTree sourceTree;
    ErrorCollector errCollector;
    m_importer = new google::protobuf::compiler::Importer(&sourceTree, &errCollector);
    m_factory = new google::protobuf::DynamicMessageFactory();

    sourceTree.MapPath("", protoRoot.getPath());
    // 遍历所有文件
    for (size_t i = 0; i < fileList.size(); ++i) {
        CFile file(fileList[i]);
        std::string ext = file.getFileExt();
        std::for_each(ext.begin(), ext.end(), [](char& c) { c = ::tolower(c); });
        // 跳过非proto文件
        if (ext != "proto") {
            continue;
        }
    
        std::string protoFileName = file.getFileName();
        if (file.getPath().length() > protoRoot.getPath().length()) {
            protoFileName = file.getPath().substr(protoRoot.getPath().length());
        }

        const google::protobuf::FileDescriptor* pFd = m_importer->Import(protoFileName);
        if (pFd) {
            m_mapfileDecs[protoFileName] = pFd;
        }
    }

    auto it = m_mapfileDecs.begin();
    for (; it != m_mapfileDecs.end(); ++it) {
        const google::protobuf::FileDescriptor* pFd = it->second;
        if (nullptr != pFd) {
            for (int j = 0; j < pFd->message_type_count(); ++j) {
                const google::protobuf::Descriptor* pDesc = pFd->message_type(j);
                if (nullptr != pDesc) {
                    MsgInfo info;
                    info.msgName = pDesc->name();
                    info.msgFullName = pDesc->full_name();
                    // 此处要特殊处理
                    if (!isFiltered(info.msgName)) {
                        registerMsgName2MsgType(info);
                    }
                }
            }
        }
    }
    return (m_mapfileDecs.size() != 0);
}

std::list<CProtoManager::MsgInfo> CProtoManager::getMsgInfos() {
    std::list<MsgInfo> listMsgs;
    for (auto pair : m_mapMsgType2MsgInfo) {
        listMsgs.emplace_back(pair.second);
    }

    return std::move(listMsgs);
}

const google::protobuf::Descriptor* CProtoManager::findMessageByName(std::string strName) {
    return m_importer->pool()->FindMessageTypeByName(strName);
}

google::protobuf::Message* CProtoManager::createMessage(const google::protobuf::Descriptor* descriptor) {
    const google::protobuf::Message* prototype = m_factory->GetPrototype(descriptor);
    if (prototype) {
        return prototype->New();
    }

    return nullptr;
}

uint16_t CProtoManager::getMsgTypeByName(std::string strName) {
    auto it = m_mapMsgType2MsgInfo.begin();
    for (; it != m_mapMsgType2MsgInfo.end(); ++it) {
        if (it->second.msgName == strName) {
            return it->first;
        }
    }

    return 0;
}

uint16_t CProtoManager::getMsgTypeByFullName(std::string strName) {
    auto it = m_mapMsgType2MsgInfo.begin();
    for (; it != m_mapMsgType2MsgInfo.end(); ++it) {
        if (it->second.msgFullName == strName) {
            return it->first;
        }
    }

    return 0;
}

void CProtoManager::registerMsgName2MsgType(const MsgInfo& msgInfo) {
    // 消息名字和消息id的对应关系为消息名字前加“_”
    const google::protobuf::EnumValueDescriptor* pEVDesc = m_importer->pool()->FindEnumValueByName("_" + msgInfo.msgName);
    if (nullptr != pEVDesc) {
        m_mapMsgType2MsgInfo[pEVDesc->number()] = msgInfo;
    }
}

CProtoManager::MsgInfo CProtoManager::getMsgInfoByMsgType(uint16_t msgType) {
    auto it = m_mapMsgType2MsgInfo.find(msgType);
    if (it != m_mapMsgType2MsgInfo.end()) {
        return it->second;
    }

    return std::move(MsgInfo());
}

std::string CProtoManager::getMsgValue(google::protobuf::Message& message, const google::protobuf::FieldDescriptor* pFd) {
    if (nullptr == pFd) {
        return "";
    }

    google::protobuf::FieldDescriptor::Type filedType = pFd->type();

    switch (filedType) {
    case google::protobuf::FieldDescriptor::TYPE_ENUM:
    {
        const google::protobuf::EnumDescriptor* pEd = pFd->enum_type();
        int enumValue = message.GetReflection()->GetEnumValue(message, pFd);
        const google::protobuf::EnumValueDescriptor* value = pEd->value(enumValue);
        return value->name();
    }
    case google::protobuf::FieldDescriptor::TYPE_INT32:
    case google::protobuf::FieldDescriptor::TYPE_INT64:
    case google::protobuf::FieldDescriptor::TYPE_UINT32:
    case google::protobuf::FieldDescriptor::TYPE_UINT64:
    case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
    case google::protobuf::FieldDescriptor::TYPE_FLOAT:
    {
        if (filedType == google::protobuf::FieldDescriptor::TYPE_INT32) {
            return std::to_string(message.GetReflection()->GetInt32(message, pFd));
        }
        else if (filedType == google::protobuf::FieldDescriptor::TYPE_UINT32) {
            return std::to_string(message.GetReflection()->GetUInt32(message, pFd));
        }
        else if (filedType == google::protobuf::FieldDescriptor::TYPE_INT64) {
            return std::to_string(message.GetReflection()->GetInt64(message, pFd));
        }
        else if (filedType == google::protobuf::FieldDescriptor::TYPE_UINT64) {
            return std::to_string(message.GetReflection()->GetUInt64(message, pFd));
        }
        else if (filedType == google::protobuf::FieldDescriptor::TYPE_DOUBLE) {
            return std::to_string(message.GetReflection()->GetDouble(message, pFd));
        }
        else if (filedType == google::protobuf::FieldDescriptor::TYPE_FLOAT) {
            return std::to_string(message.GetReflection()->GetFloat(message, pFd));
        }
    }
    case google::protobuf::FieldDescriptor::TYPE_STRING:
    case google::protobuf::FieldDescriptor::TYPE_BYTES:
    {
        return message.GetReflection()->GetString(message, pFd);
    }
    case google::protobuf::FieldDescriptor::TYPE_BOOL:
    {
        bool value = message.GetReflection()->GetBool(message, pFd);
        if (value) {
            return "true";
        }
        return "false";
    }
    case google::protobuf::FieldDescriptor::TYPE_GROUP:
        break;
    case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
    {
        const google::protobuf::Message& subMsg = message.GetReflection()->GetMessage(message, pFd);
        return subMsg.Utf8DebugString();
    }
    default:
        break;
    }
    return "";
}

std::string CProtoManager::getMsgValue(google::protobuf::Message& message, const google::protobuf::FieldDescriptor* pFd, int index) {
    if (nullptr == pFd) {
        return "";
    }

    google::protobuf::FieldDescriptor::Type filedType = pFd->type();

    switch (filedType) {
    case google::protobuf::FieldDescriptor::TYPE_ENUM:
    {
        const google::protobuf::EnumDescriptor* pEd = pFd->enum_type();
        int enumValue = message.GetReflection()->GetRepeatedEnumValue(message, pFd, index);
        const google::protobuf::EnumValueDescriptor* value = pEd->value(enumValue);
        return value->name();
    }
    case google::protobuf::FieldDescriptor::TYPE_INT32:
    case google::protobuf::FieldDescriptor::TYPE_INT64:
    case google::protobuf::FieldDescriptor::TYPE_UINT32:
    case google::protobuf::FieldDescriptor::TYPE_UINT64:
    case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
    case google::protobuf::FieldDescriptor::TYPE_FLOAT:
    {
        if (filedType == google::protobuf::FieldDescriptor::TYPE_INT32) {
            return std::to_string(message.GetReflection()->GetRepeatedInt32(message, pFd, index));
        }
        else if (filedType == google::protobuf::FieldDescriptor::TYPE_UINT32) {
            return std::to_string(message.GetReflection()->GetRepeatedUInt32(message, pFd, index));
        }
        else if (filedType == google::protobuf::FieldDescriptor::TYPE_INT64) {
            return std::to_string(message.GetReflection()->GetRepeatedInt64(message, pFd, index));
        }
        else if (filedType == google::protobuf::FieldDescriptor::TYPE_UINT64) {
            return std::to_string(message.GetReflection()->GetRepeatedUInt64(message, pFd, index));
        }
        else if (filedType == google::protobuf::FieldDescriptor::TYPE_DOUBLE) {
            return std::to_string(message.GetReflection()->GetRepeatedDouble(message, pFd, index));
        }
        else if (filedType == google::protobuf::FieldDescriptor::TYPE_FLOAT) {
            return std::to_string(message.GetReflection()->GetRepeatedFloat(message, pFd, index));
        }
    }
    case google::protobuf::FieldDescriptor::TYPE_STRING:
    case google::protobuf::FieldDescriptor::TYPE_BYTES:
    {
        return message.GetReflection()->GetRepeatedString(message, pFd, index);
    }
    case google::protobuf::FieldDescriptor::TYPE_BOOL:
    {
        bool value = message.GetReflection()->GetRepeatedBool(message, pFd, index);
        if (value) {
            return "true";
        }
        return "false";
    }
    case google::protobuf::FieldDescriptor::TYPE_GROUP:
        break;
    case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
    {
        const google::protobuf::Message& subMsg = message.GetReflection()->GetRepeatedMessage(message, pFd, index);
        return subMsg.ShortDebugString();
    }
    default:
        break;
    }
    return "";
}

bool CProtoManager::isFiltered(const std::string& strName) {
    if (strName[0] == 'M'
        && strName[1] == 'S'
        && strName[2] == 'G'
        && strName[3] == '_') {
        return false;
    }
    return true;
}
