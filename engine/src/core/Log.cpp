#include "zamin/core/Log.h"

#include <cstdarg>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

namespace zamin {

void logMessage(LogLevel level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
#if defined(__ANDROID__)
    int prio = ANDROID_LOG_INFO;
    switch (level) {
        case LogLevel::Debug: prio = ANDROID_LOG_DEBUG; break;
        case LogLevel::Info:  prio = ANDROID_LOG_INFO; break;
        case LogLevel::Warn:  prio = ANDROID_LOG_WARN; break;
        case LogLevel::Error: prio = ANDROID_LOG_ERROR; break;
    }
    __android_log_vprint(prio, "Zamin", fmt, args);
#else
    static const char* tags[] = {"D", "I", "W", "E"};
    std::fprintf(stderr, "[Zamin/%s] ", tags[static_cast<int>(level)]);
    std::vfprintf(stderr, fmt, args);
    std::fputc('\n', stderr);
#endif
    va_end(args);
}

} // namespace zamin
