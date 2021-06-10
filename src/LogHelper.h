#pragma once

#include "define.h"
#include "Singleton.h"
#include "fmt/format.h"
#include "QApplication"

enum class ELogType
{
  eLOG_TYPE_INFO = 0,
  eLOG_TYPE_WARN = 1,
  eLOG_TYPE_ERR = 2,
};

class QLogEvent final : public QEvent
{
public:
  explicit QLogEvent(const std::string& data, ELogType type)
    : QEvent(static_cast<Type>(AppEventType::eAPP_EVENT_TYPE_LOG))
    , m_message(data)
    , m_type(type) {
  }

  ELogType m_type;
  std::string m_message;
};

class CLogHelper
{
public:
  CLogHelper() = default;
  ~CLogHelper() = default;

  void setPrinter(QObject* pPrinter) { m_pPrinter = pPrinter; };

  void logScriptInfo(const char* text) {
    if (m_pPrinter) {
      QApplication::postEvent(m_pPrinter, new QLogEvent(text, ELogType::eLOG_TYPE_INFO));
    } else {
      printf("%s\n", text);
    }
  }

  void logScriptErr(const char* text) {
    if (m_pPrinter) {
      QApplication::postEvent(m_pPrinter, new QLogEvent(text, ELogType::eLOG_TYPE_ERR));
    } else {
      printf("%s\n", text);
    }
  }

  template <typename... Args>
  void logInfo(const char* text, Args&&... args);

  template <typename... Args>
  void logWarning(const char* text, Args&&... args);

  template <typename... Args>
  void logError(const char* text, Args&&... args);

  QObject* m_pPrinter = nullptr;
};

template <typename ... Args>
void CLogHelper::logInfo(const char* text, Args&&... args) {
  std::string message = fmt::format(text, args...);
  if (m_pPrinter) {
    QApplication::postEvent(m_pPrinter, new QLogEvent(message, ELogType::eLOG_TYPE_INFO));
  } else {
    printf("%s\n", message.c_str());
  }
}

template <typename ... Args>
void CLogHelper::logWarning(const char* text, Args&&... args) {
  std::string message = fmt::format(text, args...);
  if (m_pPrinter) {
    QApplication::postEvent(m_pPrinter, new QLogEvent(message, ELogType::eLOG_TYPE_WARN));
  } else {
    printf("%s\n", message.c_str());
  }
}

template <typename ... Args>
void CLogHelper::logError(const char* text, Args&&... args) {
  std::string message = fmt::format(text, args...);
  if (m_pPrinter) {
    QApplication::postEvent(m_pPrinter, new QLogEvent(message, ELogType::eLOG_TYPE_ERR));
  } else {
    printf("%s\n", message.c_str());
  }
}

#define LOG_INFO(formatStr, ...) do {\
    LogHelper::instance().logInfo(formatStr, ##__VA_ARGS__); \
} while (0)

#define LOG_WARN(formatStr, ...) do {\
    LogHelper::instance().logWarning(formatStr, ##__VA_ARGS__); \
} while (0)

#define LOG_ERR(formatStr, ...) do {\
    LogHelper::instance().logError(formatStr, ##__VA_ARGS__); \
} while (0)

typedef CSingleton<CLogHelper> LogHelper;
