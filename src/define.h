#pragma once
#include <stdint.h>
#include <cstddef>

using SocketId = uint64_t;
using Port = uint16_t;
using BuffSize = size_t;
using MessageType = uint16_t;

#ifndef WIN32
#define _atoi64(val) strtoll(val, NULL, 10)
#endif

#define MAX_BUFF_SIZE 64*1024

enum class AppEventType
{
  eAPP_EVENT_TYPE_LOG_START = 1000,
  eAPP_EVENT_TYPE_LOG = eAPP_EVENT_TYPE_LOG_START,

  eAPP_EVENT_TYPE_NET_START,
  eAPP_EVENT_TYPE_NET_CONNECT_SUCCEED,
  eAPP_EVENT_TYPE_NET_DISCONNECT,
  eAPP_EVENT_TYPE_NET_ERROR,
  eAPP_EVENT_TYPE_NET_PARSE_MESSAGE,
};

