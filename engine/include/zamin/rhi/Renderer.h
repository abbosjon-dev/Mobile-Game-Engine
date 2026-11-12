#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "zamin/scene/Camera.h"
#include "zamin/scene/Scene.h"

namespace zamin::rhi {

struct FrameParams {
    float clearColor[4] = {0.13f, 0.14f, 0.17f, 1.0f};
    EditorCamera camera;
    std::vector<DrawItem> items; // sahna obyektlari (Scene::buildDrawList)
    bool showGrid = true;
    float outlineColor[4] = {0.24f, 0.73f, 0.54f, 1.0f}; // tanlangan obyekt chegarasi (sRGB)
    float time = 0.0f;
};

// Grafik backend uchun abstrakt interfeys (hozircha faqat Vulkan).
class Renderer {
public:
    virtual ~Renderer() = default;

    // Platformaga xos oyna (Android'da ANativeWindow*).
    virtual bool attachWindow(void* nativeWindow) = 0;
    virtual void detachWindow() = 0;
    virtual void onResize(uint32_t width, uint32_t height) = 0;

    // false qaytarsa — kadr chizilmadi (oyna yo'q yoki minimallashtirilgan).
    virtual bool renderFrame(const FrameParams& params) = 0;

    virtual const char* deviceName() const = 0;

    // Ekransiz chizish (skrinshot/test). RGBA8 piksellar.
    virtual bool renderOffscreen(uint32_t width, uint32_t height, const FrameParams& params,
                                 std::vector<uint8_t>& outRgba) = 0;
};

std::unique_ptr<Renderer> createVulkanRenderer();

} // namespace zamin::rhi
