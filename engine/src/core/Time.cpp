#include "zamin/core/Time.h"

namespace zamin {

FrameTimer::FrameTimer() : m_last(Clock::now()) {}

float FrameTimer::tick() {
    const auto now = Clock::now();
    m_delta = std::chrono::duration<float>(now - m_last).count();
    m_last = now;
    // Pauza/fon holatidan keyin katta sakrashlarni cheklash
    if (m_delta > 0.25f) m_delta = 0.25f;
    m_total += m_delta;
    ++m_frameIndex;

    m_fpsAccum += m_delta;
    ++m_fpsFrames;
    if (m_fpsAccum >= 0.5f) {
        m_fps = static_cast<float>(m_fpsFrames) / m_fpsAccum;
        m_fpsAccum = 0.0f;
        m_fpsFrames = 0;
    }
    return m_delta;
}

} // namespace zamin
