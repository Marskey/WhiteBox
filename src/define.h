#pragma once
#include <stdint.h>
#include <cstddef>

using SocketId = uint64_t;
using Port = uint16_t;
using BuffSize = size_t;

#ifndef WIN32
#define _atoi64(val) strtoll(val, NULL, 10)
#endif

#define MAX_BUFF_SIZE 64*1024
