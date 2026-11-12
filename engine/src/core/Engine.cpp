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
    if (m_camera.mode2D) {
        const float aspect = m_height ? float(m_width) / float(m_height) : 1.0f;
        const float wpp = m_camera.worldPerPixel2D(aspect, m_height ? float(m_height) : 1.0f);
        m_camera.target.x -= dx * wpp;
        m_camera.target.y += dy * wpp;
        return;
    }
    constexpr float kSensitivity = 0.01f;
    m_camera.yaw += dx * kSensitivity;
    m_camera.pitch += dy * kSensitivity;
    m_camera.pitch = std::fmax(-1.45f, std::fmin(1.5f, m_camera.pitch));
}

void Engine::onPan(float dx, float dy) {
    std::lock_guard lock(m_mutex);
    const float h = m_height ? float(m_height) : 1.0f;
    const float aspect = m_height ? float(m_width) / h : 1.0f;
    if (m_camera.mode2D) {
        const float wpp = m_camera.worldPerPixel2D(aspect, h);
        m_camera.target.x -= dx * wpp;
        m_camera.target.y += dy * wpp;
        return;
    }
    // Kamera tekisligida surish: o'ng va yuqori vektorlar
    const float wpp = 2.0f * m_camera.distance * std::tan(0.45f) / h;
    const float cy = std::cos(m_camera.yaw), sy = std::sin(m_camera.yaw);
    const float cp = std::cos(m_camera.pitch), sp = std::sin(m_camera.pitch);
    const Vec3 right{cy, 0.0f, sy};
    const Vec3 up{sy * sp, cp, -cy * sp};
    m_camera.target = m_camera.target - right * (dx * wpp) + up * (dy * wpp);
}

void Engine::onZoom(float factor) {
    std::lock_guard lock(m_mutex);
    if (factor <= 0.0f) return;
    m_camera.distance = std::fmax(0.8f, std::fmin(80.0f, m_camera.distance / factor));
}

void Engine::setMode2D(bool mode2D) {
    std::lock_guard lock(m_mutex);
    m_camera.mode2D = mode2D;
}

void Engine::focus(uint32_t id) {
    std::lock_guard lock(m_mutex);
    if (!m_scene.find(id)) return;
    const Vec3 p = m_scene.worldPosition(id);
    if (m_camera.mode2D) {
        m_camera.target = {p.x, p.y, 0.0f};
    } else {
        m_camera.target = p;
    }
}

void Engine::upsertEntity(const EntityDesc& e) {
    std::lock_guard lock(m_mutex);
    m_scene.upsert(e);
}

void Engine::removeEntity(uint32_t id) {
    std::lock_guard lock(m_mutex);
    m_scene.remove(id);
    if (m_selected == id) m_selected = 0;
}

void Engine::clearScene() {
    std::lock_guard lock(m_mutex);
    m_scene.clear();
    m_selected = 0;
}

void Engine::setSelected(uint32_t id) {
    std::lock_guard lock(m_mutex);
    m_selected = id;
}

uint32_t Engine::pick(float x, float y) {
    std::lock_guard lock(m_mutex);
    Vec3 o, d;
    if (!m_camera.screenRay(x, y, float(m_width), float(m_height), o, d)) return 0;
    return m_scene.pick(o, d);
}

namespace {
// Nur va tekislik kesishuvi: tekislik normal n va nuqta p orqali
bool rayPlane(Vec3 o, Vec3 d, Vec3 n, Vec3 p, Vec3& out) {
    const float denom = dot(d, n);
    if (std::fabs(denom) < 1e-5f) return false;
    const float t = dot(p - o, n) / denom;
    if (t < 0.0f) return false;
    out = o + d * t;
    return true;
}
} // namespace

bool Engine::dragEntity(uint32_t id, float x0, float y0, float x1, float y1, Vec3& outLocalPos) {
    std::lock_guard lock(m_mutex);
    if (!m_scene.find(id)) return false;
    const Vec3 anchor = m_scene.worldPosition(id);
    const Vec3 normal = m_camera.mode2D ? Vec3{0, 0, 1} : Vec3{0, 1, 0};
    Vec3 o0, d0, o1, d1, p0, p1;
    const float w = float(m_width), h = float(m_height);
    if (!m_camera.screenRay(x0, y0, w, h, o0, d0) || !m_camera.screenRay(x1, y1, w, h, o1, d1))
        return false;
    if (!rayPlane(o0, d0, normal, anchor, p0) || !rayPlane(o1, d1, normal, anchor, p1)) return false;
    Vec3 delta = p1 - p0;
    // Ufqqa yaqin burchakda juda katta sakrashlarni cheklaymiz
    const float maxStep = m_camera.distance * 0.5f;
    if (length(delta) > maxStep) delta = normalize(delta) * maxStep;
    if (!m_scene.translateWorld(id, delta)) return false;
    outLocalPos = m_scene.find(id)->position;
    return true;
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

            m_timer.tick();
            params.camera = m_camera;
            m_scene.buildDrawList(m_drawList, m_selected);
            params.items = m_drawList;
            params.time = static_cast<float>(m_timer.totalSeconds());
        }

        m_renderer->renderFrame(params);

        std::lock_guard lock(m_mutex);
        m_stats.fps = m_timer.fps();
        m_stats.frameMs = m_timer.deltaSeconds() * 1000.0f;
        m_stats.frames = m_timer.frameIndex();
        m_stats.cameraYaw = m_camera.mode2D ? 0.0f : m_camera.yaw;
        m_stats.cameraPitch = m_camera.mode2D ? 0.0f : m_camera.pitch;
        m_stats.drawCalls = static_cast<uint32_t>(params.items.size()) + 1;
    }

    if (m_hasWindow) m_renderer->detachWindow();
    m_hasWindow = false;
    m_renderer.reset();
    std::lock_guard lock(m_mutex);
    m_windowChanged = false;
    m_cv.notify_all();
}

} // namespace zamin
