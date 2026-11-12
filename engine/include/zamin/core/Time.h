#pragma once

#include <chrono>
#include <cstdint>

namespace zamin {

// Kadrlar orasidagi vaqtni va FPS ni hisoblaydi.
class FrameTimer {
public:
    FrameTimer();

    // Har kadr boshida chaqiriladi. Delta (soniyalarda) qaytaradi.
    float tick();

    float deltaSeconds() const { return m_delta; }
    double totalSeconds() const { return m_total; }
    float fps() const { return m_fps; }
    uint64_t frameIndex() const { return m_frameIndex; }

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point m_last;
    float m_delta = 0.0f;
    double m_total = 0.0;
    float m_fps = 0.0f;
    float m_fpsAccum = 0.0f;
    uint32_t m_fpsFrames = 0;
    uint64_t m_frameIndex = 0;
};

} // namespace zamin
