#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "zamin/core/Time.h"

namespace zamin {

namespace rhi { class Renderer; }

struct EngineStats {
    float fps = 0.0f;
    float frameMs = 0.0f;
    uint64_t frames = 0;
};

// Engine — o'z render oqimida (thread) ishlaydi.
// Barcha public metodlar UI oqimidan xavfsiz chaqiriladi.
class Engine {
public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void start();
    void stop();

    // Oynani biriktiradi/ajratadi. detach render oqimi oynadan
    // foydalanishni to'xtatmaguncha bloklaydi (Android talabi).
    void setWindow(void* nativeWindow);
    void onResize(uint32_t width, uint32_t height);
    void setPaused(bool paused);

    // Sensorli boshqaruv: dx/dy — piksellarda siljish.
    void onDrag(float dx, float dy);

    EngineStats stats() const;
    std::string deviceName() const;

private:
    void renderLoop();

    std::unique_ptr<rhi::Renderer> m_renderer;
    std::thread m_thread;

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_running = false;
    bool m_paused = false;
    void* m_pendingWindow = nullptr;
    bool m_windowChanged = false;
    bool m_hasWindow = false;
    uint32_t m_width = 0, m_height = 0;
    bool m_resized = false;

    float m_rotX = 0.0f, m_rotY = 0.0f;
    float m_velY = 0.6f;

    EngineStats m_stats;
    std::string m_deviceName;
    FrameTimer m_timer;
};

} // namespace zamin
