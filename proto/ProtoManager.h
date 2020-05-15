#pragma once

#include "google/protobuf/compiler/importer.h"
#include "libCommon/Template/Singleton.h"

class ErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector
{
public:
    ErrorCollector() /*:m_nErrCnt(0), m_nWarnCnt(0)*/ {}
    virtual ~ErrorCollector() {};
    // Line and column numbers are zero-based.  A line number of -1 indicates
    // an error with the entire file (e.g. "not found").
    virtual void AddError(const std::string& filename, int line, int column, const std::string& message) {
        //printf("[ERROR] file: %s(%d, %d)\nMessage: %s\n", filename.c_str(), line, column, message.c_str());
        //m_nErrCnt++;
    }

    virtual void AddWarning(const std::string& filename, int line, int column, const std::string& message) {
        //printf("[WARN] file: %s(%d, %d)\nMessage: %s\n", filename.c_str(), line, column, message.c_str());
        //m_nWarnCnt++;
    }

    void printResult() {
        //printf("error(%d), warning(%d)\n", m_nErrCnt, m_nWarnCnt);
    }

private:
    //int m_nErrCnt;
    //int m_nWarnCnt;
};

class CProtoManager
{
public:
    struct MsgInfo
    {
        std::string msgFullName;
        std::string msgName;
        std::string packageName;

        friend bool operator==(const MsgInfo& l, const MsgInfo& r) {
            if (l.msgFullName == r.msgFullName
                && l.msgName == r.msgName
                && l.packageName == r.packageName) {
                return true;
            }

            return false;
        }
    };

public:
    CProtoManager();
    ~CProtoManager();

    bool importMsg(const char* path, const char* rootPath);
    std::list<MsgInfo> getMsgInfos();

    const google::protobuf::Descriptor* findMessageByName(std::string strName);

    google::protobuf::Message* createMessage(const google::protobuf::Descriptor * pDescriptor);

    uint16_t getMsgTypeByName(std::string strName);
    uint16_t getMsgTypeByFullName(std::string strName);

    MsgInfo getMsgInfoByMsgType(uint16_t msgType);

    std::string getMsgValue(google::protobuf::Message& message, const google::protobuf::FieldDescriptor* pFd);

    std::string getMsgValue(google::protobuf::Message& message, const google::protobuf::FieldDescriptor* pFd, int index);

private:
    bool isFiltered(const std::string& strName);
    void registerMsgName2MsgType(const MsgInfo& msgInfo);

private:
    google::protobuf::compiler::Importer* m_importer;
    google::protobuf::DynamicMessageFactory* m_factory;
    std::unordered_map<std::string, const google::protobuf::FileDescriptor*> m_mapfileDecs;
    std::unordered_map<uint16_t, MsgInfo> m_mapMsgType2MsgInfo;
};

typedef CSingleton<CProtoManager> ProtoManager;
