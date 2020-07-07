#pragma once

#include "LuaInterface.h"
#include "LogHelper.h"

#include "google/protobuf/compiler/importer.h"
#include "Singleton.h"

class CProtoManager : public lua_api::IProtoManager
{
    class MultiFileErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector
    {
    public:
        void AddError(const std::string& filename, int line, int column,
                      const std::string& message) override {

            LogHelper::instance().logError("Error: import {} failed, line: {}, col: {}. {}", filename, line, column, message);
        }
    };

    struct ImporterInfo
    {
        google::protobuf::compiler::Importer* pImporter;
        google::protobuf::compiler::DiskSourceTree* pSourceTree;
        google::protobuf::DynamicMessageFactory* pFactory;
        MultiFileErrorCollector* pCollector;
    };

public:
    struct MsgInfo
    {
        int32_t msgType;
        std::string msgFullName;
        std::string msgName;

        friend bool operator==(const MsgInfo& l, const MsgInfo& r) {
            if (l.msgFullName == r.msgFullName
                && l.msgName == r.msgName
                && l.msgType == r.msgType) {
                return true;
            }

            return false;
        }
    };

public:
    //CProtoManager() = default;
    CProtoManager();
    ~CProtoManager();

    bool init(const std::string& rootPath);

    bool importProto(const std::string& virtualFilePath);

    std::list<MsgInfo> getMsgInfos();

    const google::protobuf::Descriptor* findMessageByName(const std::string& strName);

    google::protobuf::Message* createMessage(const char* msgFullName);

    uint16_t getMsgTypeByFullName(const std::string& strName);

    const MsgInfo* getMsgInfoByMsgType(uint16_t msgType);

    std::string getMsgValue(google::protobuf::Message& message, const google::protobuf::FieldDescriptor* pFd);
    std::string getMsgValue(google::protobuf::Message& message, const google::protobuf::FieldDescriptor* pFd, int index);

    // lua_api::IProtoManager begin
    void addProtoMessage(int msgType, const char* msgFullName, const char* msgName) override;
    void* getDescriptorPool() override;
    // lua_api::IProtoManager end
private:
    ImporterInfo m_importerInfo;
    std::unordered_map<std::string, MsgInfo> m_mapMsgFullName2MsgInfo;
};

typedef CSingleton<CProtoManager> ProtoManager;
