#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace zamin::rhi {

struct FrameParams {
    float clearColor[4] = {0.09f, 0.10f, 0.12f, 1.0f};
    float rotationY = 0.0f;   // radian
    float rotationX = 0.0f;   // radian
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
