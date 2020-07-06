#include "ProtoManager.h"
#include "google/protobuf/dynamic_message.h"

CProtoManager::~CProtoManager() {
    delete m_importerInfo.pFactory;
    m_importerInfo.pFactory = nullptr;
    delete m_importerInfo.pSourceTree;
    m_importerInfo.pSourceTree = nullptr;
    delete m_importerInfo.pCollector;
    m_importerInfo.pCollector = nullptr;
    delete m_importerInfo.pImporter;
    m_importerInfo.pImporter = nullptr;
}

bool CProtoManager::init(const std::string& rootPath) {
    m_importerInfo.pSourceTree = new google::protobuf::compiler::DiskSourceTree();
    m_importerInfo.pSourceTree->MapPath("", rootPath);
    m_importerInfo.pCollector = new MultiFileErrorCollector();
    m_importerInfo.pImporter = new google::protobuf::compiler::Importer(m_importerInfo.pSourceTree, m_importerInfo.pCollector);
    m_importerInfo.pFactory = new google::protobuf::DynamicMessageFactory();
    return true;
}

bool CProtoManager::importProto(const std::string& virtualFilePath) {
    if (virtualFilePath.empty()) {
        return false;
    }

    return nullptr != m_importerInfo.pImporter->Import(virtualFilePath);
}

std::list<CProtoManager::MsgInfo> CProtoManager::getMsgInfos() {
    std::list<MsgInfo> listMsgs;
    for (const auto& pair : m_mapMsgType2MsgInfo) {
        listMsgs.emplace_back(pair.second);
    }

    return std::move(listMsgs);
}

const google::protobuf::Descriptor* CProtoManager::findMessageByName(const std::string& strName) {
    return m_importerInfo.pImporter->pool()->FindMessageTypeByName(strName);
}

google::protobuf::Message* CProtoManager::createMessage(const char* msgFullName) {
    auto* descriptor = m_importerInfo.pImporter->pool()->FindMessageTypeByName(msgFullName);
    if (nullptr == descriptor) {
        return nullptr;
    }

    const google::protobuf::Message* prototype = m_importerInfo.pFactory->GetPrototype(descriptor);
    if (prototype) {
        return prototype->New();
    }

    return nullptr;
}

uint16_t CProtoManager::getMsgTypeByFullName(const std::string& strName) {
    auto it = m_mapMsgType2MsgInfo.begin();
    for (; it != m_mapMsgType2MsgInfo.end(); ++it) {
        if (it->second.msgFullName == strName) {
            return it->first;
        }
    }

    return 0;
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
            if (filedType == google::protobuf::FieldDescriptor::TYPE_UINT32) {
                return std::to_string(message.GetReflection()->GetUInt32(message, pFd));
            }
            if (filedType == google::protobuf::FieldDescriptor::TYPE_INT64) {
                return std::to_string(message.GetReflection()->GetInt64(message, pFd));
            }
            if (filedType == google::protobuf::FieldDescriptor::TYPE_UINT64) {
                return std::to_string(message.GetReflection()->GetUInt64(message, pFd));
            }
            if (filedType == google::protobuf::FieldDescriptor::TYPE_DOUBLE) {
                return std::to_string(message.GetReflection()->GetDouble(message, pFd));
            }
            if (filedType == google::protobuf::FieldDescriptor::TYPE_FLOAT) {
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
            if (filedType == google::protobuf::FieldDescriptor::TYPE_UINT32) {
                return std::to_string(message.GetReflection()->GetRepeatedUInt32(message, pFd, index));
            }
            if (filedType == google::protobuf::FieldDescriptor::TYPE_INT64) {
                return std::to_string(message.GetReflection()->GetRepeatedInt64(message, pFd, index));
            }
            if (filedType == google::protobuf::FieldDescriptor::TYPE_UINT64) {
                return std::to_string(message.GetReflection()->GetRepeatedUInt64(message, pFd, index));
            }
            if (filedType == google::protobuf::FieldDescriptor::TYPE_DOUBLE) {
                return std::to_string(message.GetReflection()->GetRepeatedDouble(message, pFd, index));
            }
            if (filedType == google::protobuf::FieldDescriptor::TYPE_FLOAT) {
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

void CProtoManager::addProtoMessage(int msgType, const char* msgFullName, const char* msgName) {
    MsgInfo msg{ msgFullName, msgName };
    m_mapMsgType2MsgInfo[msgType] = msg;
}

void* CProtoManager::getDescriptorPool() {
    return (void*)m_importerInfo.pImporter->pool();
}
