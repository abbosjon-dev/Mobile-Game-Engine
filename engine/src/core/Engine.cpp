#include "zamin/core/Engine.h"

#include <cmath>

#include "zamin/core/Log.h"
#include "zamin/rhi/Renderer.h"

namespace zamin {

Engine::Engine() = default;

Engine::~Engine() { stop(); }

void Engine::start() {
    std::lock_guard lock(m_mutex);
    if (m_running) return;
    m_running = true;
    m_thread = std::thread(&Engine::renderLoop, this);
    ZAMIN_LOGI("Engine started");
}

void Engine::stop() {
    {
        std::lock_guard lock(m_mutex);
        if (!m_running) return;
        m_running = false;
    }
    m_cv.notify_all();
    if (m_thread.joinable()) m_thread.join();
    ZAMIN_LOGI("Engine stopped");
}

void Engine::setWindow(void* nativeWindow) {
    std::unique_lock lock(m_mutex);
    m_pendingWindow = nativeWindow;
    m_windowChanged = true;
    m_cv.notify_all();
    // Render oqimi o'zgarishni qabul qilmaguncha kutamiz.
    // Bu ayniqsa surfaceDestroyed uchun muhim.
    m_cv.wait(lock, [this] { return !m_windowChanged || !m_running; });
}

void Engine::onResize(uint32_t width, uint32_t height) {
    std::lock_guard lock(m_mutex);
    m_width = width;
    m_height = height;
    m_resized = true;
    m_cv.notify_all();
}

void Engine::setPaused(bool paused) {
    std::lock_guard lock(m_mutex);
    m_paused = paused;
    m_cv.notify_all();
}

void Engine::onDrag(float dx, float dy) {
    std::lock_guard lock(m_mutex);
    constexpr float kSensitivity = 0.01f;
    m_rotY += dx * kSensitivity;
    m_rotX += dy * kSensitivity;
    m_rotX = std::fmax(-1.4f, std::fmin(1.4f, m_rotX));
    m_velY = 0.0f; // foydalanuvchi boshqarsa avtomatik aylanish to'xtaydi
}

EngineStats Engine::stats() const {
    std::lock_guard lock(m_mutex);
    return m_stats;
}

std::string Engine::deviceName() const {
    std::lock_guard lock(m_mutex);
    return m_deviceName;
}

void Engine::renderLoop() {
    m_renderer = rhi::createVulkanRenderer();

    while (true) {
        rhi::FrameParams params;
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [this] {
                return !m_running || m_windowChanged || (m_hasWindow && !m_paused);
            });
            if (!m_running) break;

            if (m_windowChanged) {
                if (m_hasWindow) {
                    m_renderer->detachWindow();
                    m_hasWindow = false;
                }
                if (m_pendingWindow) {
                    m_hasWindow = m_renderer && m_renderer->attachWindow(m_pendingWindow);
                    if (m_hasWindow) m_deviceName = m_renderer->deviceName();
                }
                m_windowChanged = false;
                m_cv.notify_all();
                m_timer.tick(); // vaqt sakrashini oldini olish
                continue;
            }
            if (m_resized) {
                m_renderer->onResize(m_width, m_height);
                m_resized = false;
            }

            const float dt = m_timer.tick();
            m_rotY += m_velY * dt;
            params.rotationX = m_rotX;
            params.rotationY = m_rotY;
            params.time = static_cast<float>(m_timer.totalSeconds());
        }

        m_renderer->renderFrame(params);

        std::lock_guard lock(m_mutex);
        m_stats.fps = m_timer.fps();
        m_stats.frameMs = m_timer.deltaSeconds() * 1000.0f;
        m_stats.frames = m_timer.frameIndex();
    }

    if (m_hasWindow) m_renderer->detachWindow();
    m_hasWindow = false;
    m_renderer.reset();
    std::lock_guard lock(m_mutex);
    m_windowChanged = false;
    m_cv.notify_all();
}

} // namespace zamin
