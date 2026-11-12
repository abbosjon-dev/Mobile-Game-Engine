#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "zamin/core/Time.h"
#include "zamin/scene/Camera.h"
#include "zamin/scene/Scene.h"

namespace zamin {

namespace rhi { class Renderer; }

struct EngineStats {
    float fps = 0.0f;
    float frameMs = 0.0f;
    uint64_t frames = 0;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    uint32_t drawCalls = 0;
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
    // 3D: kamerani nishon atrofida aylantirish, 2D: surish (pan).
    void onDrag(float dx, float dy);
    // Ikki barmoq bilan surish (3D da ham pan).
    void onPan(float dx, float dy);
    // Ikki barmoq: masshtab (pinch). factor > 1 — yaqinlashtirish.
    void onZoom(float factor);
    void setMode2D(bool mode2D);
    // Kamerani obyektga qaratish
    void focus(uint32_t id);

    // ---- Sahna ----
    void upsertEntity(const EntityDesc& e);
    void removeEntity(uint32_t id);
    void clearScene();
    void setSelected(uint32_t id);
    // Ekran nuqtasidagi obyekt (piksel); 0 — hech narsa
    uint32_t pick(float x, float y);
    // Obyektni barmoq ortidan siljitish: 3D da gorizontal tekislikda, 2D da XY da.
    // Muvaffaqiyatli bo'lsa yangi lokal pozitsiyani qaytaradi.
    bool dragEntity(uint32_t id, float x0, float y0, float x1, float y1, Vec3& outLocalPos);

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

    EditorCamera m_camera;
    Scene m_scene;
    uint32_t m_selected = 0;
    std::vector<DrawItem> m_drawList;

    EngineStats m_stats;
    std::string m_deviceName;
    FrameTimer m_timer;
};

} // namespace zamin
