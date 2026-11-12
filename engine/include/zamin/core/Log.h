#pragma once

#include <cstdio>

namespace zamin {

enum class LogLevel { Debug, Info, Warn, Error };

void logMessage(LogLevel level, const char* fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

} // namespace zamin

#define ZAMIN_LOGD(...) ::zamin::logMessage(::zamin::LogLevel::Debug, __VA_ARGS__)
#define ZAMIN_LOGI(...) ::zamin::logMessage(::zamin::LogLevel::Info, __VA_ARGS__)
#define ZAMIN_LOGW(...) ::zamin::logMessage(::zamin::LogLevel::Warn, __VA_ARGS__)
#define ZAMIN_LOGE(...) ::zamin::logMessage(::zamin::LogLevel::Error, __VA_ARGS__)
