#pragma once

#include "toolkit/Singleton.h"
#include "toolkit/fmt/format.h"

class CLogPrinter
{
public:
    CLogPrinter() = default;
    virtual ~CLogPrinter() = default;
    virtual void onPrintDebug(const std::string& message) {};
    virtual void onPrintInfo(const std::string& message) {};
    virtual void onPrintWarning(const std::string& message) {};
    virtual void onPrintError(const std::string& message) {};
};

class CLogHelper
{
public:
    CLogHelper() = default;
    ~CLogHelper() = default; 

    void setPrinter(CLogPrinter* pPrinter) { m_pPrinter = pPrinter; };

    template <typename... Args>
    void logDebug(const char* text, Args&&... args);

    template <typename... Args>
    void logInfo(const char* text, Args&&... args);

    template <typename... Args>
    void logWarning(const char* text, Args&&... args);

    template <typename... Args>
    void logError(const char* text, Args&&... args);

    CLogPrinter* m_pPrinter = nullptr;
};

template <typename ... Args>
void CLogHelper::logDebug(const char* text, Args&&... args) {
    std::string message = fmt::format(text, args...);
    if (m_pPrinter) {
        m_pPrinter->onPrintInfo(message);
    } else {
        printf(message.c_str());
    }
}

template <typename ... Args>
void CLogHelper::logInfo(const char* text, Args&&... args) {
    std::string message = fmt::format(text, args...);
    if (m_pPrinter) {
        m_pPrinter->onPrintInfo(message);
    } else {
        printf(message.c_str());
    }
}

template <typename ... Args>
void CLogHelper::logWarning(const char* text, Args&&... args) {
    std::string message = fmt::format(text, args...);
    if (m_pPrinter) {
        m_pPrinter->onPrintWarning(message);
    } else {
        printf(message.c_str());
    }
}

template <typename ... Args>
void CLogHelper::logError(const char* text, Args&&... args) {
    std::string message = fmt::format(text, args...);
    if (m_pPrinter) {
        m_pPrinter->onPrintError(message);
    } else {
        printf(message.c_str());
    }
}

#define LOG_DEG(formatStr, ...) do {\
    LogHelper::singleton().logDebug(formatStr, __VA_ARGS__); \
} while (0)

#define LOG_INFO(formatStr, ...) do {\
    LogHelper::singleton().logInfo(formatStr, __VA_ARGS__); \
} while (0)

#define LOG_WARN(formatStr, ...) do {\
    LogHelper::singleton().logWarning(formatStr, __VA_ARGS__); \
} while (0)

#define LOG_ERR(formatStr, ...) do {\
    LogHelper::singleton().logError(formatStr, __VA_ARGS__); \
} while (0)

typedef CSingleton<CLogHelper> LogHelper;
