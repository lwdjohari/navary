#pragma once
#include <cstdio>
#include <ctime>

#include "navary/macro.h"

NVR_BEGIN_NAMESPACE

// Simple log level macros — all print to stderr for now.
// In future, we will route these to spdlog, absl::Log, or our engine logger.

#define NVR_LOG_TIMESTAMP()                                                    \
  do {                                                                         \
    std::time_t _t = std::time(nullptr);                                       \
    std::tm _tm{};                                                             \
    localtime_r(&_t, &_tm);                                                    \
    std::fprintf(stderr, "[%04d-%02d-%02d %02d:%02d:%02d] ",                   \
                 1900 + _tm.tm_year, _tm.tm_mon + 1, _tm.tm_mday, _tm.tm_hour, \
                 _tm.tm_min, _tm.tm_sec);                                      \
  } while (0)

#define NVR_LOG_FMT(level, section, msg)                           \
  do {                                                             \
    NVR_LOG_TIMESTAMP();                                           \
    std::fprintf(stderr, "[%-5s] [%s] %s\n", level, section, msg); \
  } while (0)

// ---- User-facing logging macros ----
#define NVR_LFATAL(section, msg) NVR_LOG_FMT("FATAL", section, msg)
#define NVR_LINFO(section, msg) NVR_LOG_FMT("INFO", section, msg)
#define NVR_LWARN(section, msg) NVR_LOG_FMT("WARN", section, msg)
#define NVR_LERROR(section, msg) NVR_LOG_FMT("ERROR", section, msg)
#define NVR_LDEBUG(section, msg) NVR_LOG_FMT("DEBUG", section, msg)
#define NVR_LTRACE(section, msg) NVR_LOG_FMT("TRACE", section, msg)

NVR_END_NAMESPACE