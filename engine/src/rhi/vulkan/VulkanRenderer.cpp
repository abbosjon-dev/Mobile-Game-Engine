// Zamin — Vulkan renderer (Bosqich 0)
// Instance/device yaratish, swapchain (Android pre-rotation bilan),
// render pass va oddiy 3D aylanuvchi piramida chizish.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#include <android/native_window.h>
#endif
#include <vulkan/vulkan.h>

#include "zamin/core/Log.h"
#include "zamin/rhi/Renderer.h"

namespace zamin::rhi {
namespace {

// Build vaqtida glslc tomonidan generatsiya qilinadi (SPIR-V so'zlar ro'yxati)
const uint32_t kBasicVertSpv[] = {
#include "basic.vert.spv.inc"
};
const uint32_t kBasicFragSpv[] = {
#include "basic.frag.spv.inc"
};
const uint32_t kGridVertSpv[] = {
#include "grid.vert.spv.inc"
};
const uint32_t kGridFragSpv[] = {
#include "grid.frag.spv.inc"
};

#define VK_CHECK(expr)                                                        \
    do {                                                                      \
        VkResult _r = (expr);                                                 \
        if (_r != VK_SUCCESS) {                                               \
            ZAMIN_LOGE("%s failed: %d (%s:%d)", #expr, _r, __FILE__, __LINE__); \
            return false;                                                     \
        }                                                                     \
    } while (0)

constexpr uint32_t kFramesInFlight = 2;

struct PushConstants {
    float mvp[16];
    float params[4]; // x: 1 = chiqish sRGB (rangni linearga o'tkazish kerak)
};

bool isSrgb(VkFormat f) {
    return f == VK_FORMAT_R8G8B8A8_SRGB || f == VK_FORMAT_B8G8R8A8_SRGB;
}

// Ranglar sRGB da beriladi; sRGB framebuffer'ga linear qiymat yozilishi kerak.
float srgbToLinear(float c) {
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

// --- Kichik matritsa yordamchilari (column-major, Vulkan clip space) ---
struct Mat4 {
    float m[16];
};

Mat4 identity() {
    Mat4 r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

Mat4 mul(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int rr = 0; rr < 4; ++rr) {
            float s = 0.0f;
            for (int k = 0; k < 4; ++k) s += a.m[k * 4 + rr] * b.m[c * 4 + k];
            r.m[c * 4 + rr] = s;
        }
    return r;
}

Mat4 rotationX(float a) {
    Mat4 r = identity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[5] = c; r.m[6] = s; r.m[9] = -s; r.m[10] = c;
    return r;
}

Mat4 rotationY(float a) {
    Mat4 r = identity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[0] = c; r.m[2] = -s; r.m[8] = s; r.m[10] = c;
    return r;
}

Mat4 rotationZ(float a) {
    Mat4 r = identity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[0] = c; r.m[1] = s; r.m[4] = -s; r.m[5] = c;
    return r;
}

Mat4 translation(float x, float y, float z) {
    Mat4 r = identity();
    r.m[12] = x; r.m[13] = y; r.m[14] = z;
    return r;
}

// Right-handed, depth 0..1, Vulkan Y pastga
Mat4 perspective(float fovY, float aspect, float zNear, float zFar) {
    Mat4 r{};
    const float f = 1.0f / std::tan(fovY * 0.5f);
    r.m[0] = f / aspect;
    r.m[5] = -f;
    r.m[10] = zFar / (zNear - zFar);
    r.m[11] = -1.0f;
    r.m[14] = (zNear * zFar) / (zNear - zFar);
    return r;
}

class VulkanRenderer final : public Renderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() override {
        detachWindow();
        destroyDevice();
    }

    bool attachWindow(void* nativeWindow) override {
        if (!m_instance && !createInstance()) return false;
#if defined(__ANDROID__)
        auto* window = static_cast<ANativeWindow*>(nativeWindow);
        VkAndroidSurfaceCreateInfoKHR sci{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
        sci.window = window;
        VK_CHECK(vkCreateAndroidSurfaceKHR(m_instance, &sci, nullptr, &m_surface));
#else
        (void)nativeWindow;
        ZAMIN_LOGE("Bu platformada oyna yaratish hali qo'llab-quvvatlanmaydi");
        return false;
#endif
        if (!m_device && !createDevice()) {
            destroySurface();
            return false;
        }
        if (!createSwapchain()) {
            destroySurface();
            return false;
        }
        ZAMIN_LOGI("Window attached: %ux%u", m_extent.width, m_extent.height);
        return true;
    }

    void detachWindow() override {
        if (!m_device) {
            destroySurface();
            return;
        }
        vkDeviceWaitIdle(m_device);
        destroySwapchain();
        destroySurface();
    }

    void onResize(uint32_t, uint32_t) override { m_needsRecreate = true; }

    bool renderFrame(const FrameParams& params) override {
        if (!m_swapchain) return false;
        if (m_needsRecreate) {
            if (!recreateSwapchain()) return false;
        }

        Frame& frame = m_frames[m_frameIndex];
        VK_CHECK(vkWaitForFences(m_device, 1, &frame.fence, VK_TRUE, UINT64_MAX));

        uint32_t imageIndex = 0;
        VkResult acq = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
                                             frame.imageAvailable, VK_NULL_HANDLE, &imageIndex);
        if (acq == VK_ERROR_OUT_OF_DATE_KHR) {
            m_needsRecreate = true;
            return false;
        }
        if (acq != VK_SUCCESS && acq != VK_SUBOPTIMAL_KHR) {
            ZAMIN_LOGE("vkAcquireNextImageKHR: %d", acq);
            return false;
        }
        VK_CHECK(vkResetFences(m_device, 1, &frame.fence));

        recordCommands(frame.cmd, imageIndex, params);

        const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &frame.imageAvailable;
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &frame.cmd;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &m_images[imageIndex].renderFinished;
        VK_CHECK(vkQueueSubmit(m_queue, 1, &submit, frame.fence));

        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &m_images[imageIndex].renderFinished;
        present.swapchainCount = 1;
        present.pSwapchains = &m_swapchain;
        present.pImageIndices = &imageIndex;
        VkResult pres = vkQueuePresentKHR(m_queue, &present);
        // Android'da SUBOPTIMAL ko'pincha pre-rotation bilan bog'liq;
        // biz uni o'zimiz boshqaramiz, shuning uchun faqat OUT_OF_DATE da qayta yaratamiz.
        if (pres == VK_ERROR_OUT_OF_DATE_KHR) m_needsRecreate = true;

        m_frameIndex = (m_frameIndex + 1) % kFramesInFlight;
        return true;
    }

    const char* deviceName() const override { return m_deviceName; }

    // Ekransiz (offscreen) chizish: skrinshot va testlar uchun.
    // Natija: RGBA8 (sRGB) piksellar, yuqori-chapdan boshlab.
    bool renderOffscreen(uint32_t width, uint32_t height, const FrameParams& params,
                         std::vector<uint8_t>& outRgba) override {
        m_headless = true;
        if (!m_instance && !createInstance()) return false;
        if (!m_device && !createDevice()) return false;
        m_format = VK_FORMAT_R8G8B8A8_SRGB;
        m_extent = {width, height};
        m_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        if (!createDepth()) return false;
        if (!m_renderPass && !createRenderPass()) return false;
        if (!m_pipeline && !createPipeline()) return false;

        // Rang tasviri
        VkImage color = VK_NULL_HANDLE;
        VkDeviceMemory colorMem = VK_NULL_HANDLE;
        VkImageView colorView = VK_NULL_HANDLE;
        VkFramebuffer fb = VK_NULL_HANDLE;
        VkBuffer readback = VK_NULL_HANDLE;
        VkDeviceMemory readbackMem = VK_NULL_HANDLE;
        const VkDeviceSize bytes = VkDeviceSize(width) * height * 4;

        VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ici.imageType = VK_IMAGE_TYPE_2D;
        ici.format = m_format;
        ici.extent = {width, height, 1};
        ici.mipLevels = 1;
        ici.arrayLayers = 1;
        ici.samples = VK_SAMPLE_COUNT_1_BIT;
        ici.tiling = VK_IMAGE_TILING_OPTIMAL;
        ici.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VK_CHECK(vkCreateImage(m_device, &ici, nullptr, &color));
        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(m_device, color, &req);
        VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        mai.allocationSize = req.size;
        mai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK(vkAllocateMemory(m_device, &mai, nullptr, &colorMem));
        VK_CHECK(vkBindImageMemory(m_device, color, colorMem, 0));

        VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vci.image = color;
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format = m_format;
        vci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VK_CHECK(vkCreateImageView(m_device, &vci, nullptr, &colorView));

        VkImageView attachments[] = {colorView, m_depthView};
        VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fci.renderPass = m_renderPass;
        fci.attachmentCount = 2;
        fci.pAttachments = attachments;
        fci.width = width;
        fci.height = height;
        fci.layers = 1;
        VK_CHECK(vkCreateFramebuffer(m_device, &fci, nullptr, &fb));

        VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bci.size = bytes;
        bci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VK_CHECK(vkCreateBuffer(m_device, &bci, nullptr, &readback));
        vkGetBufferMemoryRequirements(m_device, readback, &req);
        mai.allocationSize = req.size;
        mai.memoryTypeIndex = findMemoryType(req.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VK_CHECK(vkAllocateMemory(m_device, &mai, nullptr, &readbackMem));
        VK_CHECK(vkBindBufferMemory(m_device, readback, readbackMem, 0));

        Frame& frame = m_frames[0];
        VK_CHECK(vkWaitForFences(m_device, 1, &frame.fence, VK_TRUE, UINT64_MAX));
        VK_CHECK(vkResetFences(m_device, 1, &frame.fence));
        recordScene(frame.cmd, fb, params, /*end=*/false);

        // Render pass oxirida tasvir TRANSFER_SRC holatida bo'ladi
        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {width, height, 1};
        vkCmdCopyImageToBuffer(frame.cmd, color, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback, 1, &region);
        vkEndCommandBuffer(frame.cmd);

        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &frame.cmd;
        VK_CHECK(vkQueueSubmit(m_queue, 1, &submit, frame.fence));
        VK_CHECK(vkWaitForFences(m_device, 1, &frame.fence, VK_TRUE, UINT64_MAX));

        void* mapped = nullptr;
        VK_CHECK(vkMapMemory(m_device, readbackMem, 0, bytes, 0, &mapped));
        outRgba.resize(bytes);
        std::memcpy(outRgba.data(), mapped, bytes);
        vkUnmapMemory(m_device, readbackMem);

        vkDestroyBuffer(m_device, readback, nullptr);
        vkFreeMemory(m_device, readbackMem, nullptr);
        vkDestroyFramebuffer(m_device, fb, nullptr);
        vkDestroyImageView(m_device, colorView, nullptr);
        vkDestroyImage(m_device, color, nullptr);
        vkFreeMemory(m_device, colorMem, nullptr);
        destroySwapchain(); // depth resurslarini tozalaydi
        return true;
    }

private:
    struct Frame {
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
    };
    struct SwapImage {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkSemaphore renderFinished = VK_NULL_HANDLE;
    };

    // ------------------------------------------------------------------
    bool createInstance() {
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName = "Zamin Editor";
        app.pEngineName = "Zamin";
        app.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        app.apiVersion = VK_API_VERSION_1_1;

        std::vector<const char*> extensions;
        if (!m_headless) {
            extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(__ANDROID__)
            extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif
        }
        std::vector<const char*> layers;
#if !defined(NDEBUG)
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> available(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, available.data());
        for (auto& l : available) {
            if (std::strcmp(l.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                layers.push_back("VK_LAYER_KHRONOS_validation");
                ZAMIN_LOGI("Vulkan validation layer yoqildi");
            }
        }
#endif
        VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        ci.pApplicationInfo = &app;
        ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        ci.ppEnabledExtensionNames = extensions.data();
        ci.enabledLayerCount = static_cast<uint32_t>(layers.size());
        ci.ppEnabledLayerNames = layers.data();
        VK_CHECK(vkCreateInstance(&ci, nullptr, &m_instance));
        return true;
    }

    bool createDevice() {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
        if (count == 0) {
            ZAMIN_LOGE("Vulkan qurilma topilmadi");
            return false;
        }
        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

        for (VkPhysicalDevice pd : devices) {
            uint32_t qCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(pd, &qCount, nullptr);
            std::vector<VkQueueFamilyProperties> families(qCount);
            vkGetPhysicalDeviceQueueFamilyProperties(pd, &qCount, families.data());
            for (uint32_t i = 0; i < qCount; ++i) {
                VkBool32 present = m_headless ? VK_TRUE : VK_FALSE;
                if (!m_headless) vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, m_surface, &present);
                if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
                    m_physical = pd;
                    m_queueFamily = i;
                    break;
                }
            }
            if (m_physical) break;
        }
        if (!m_physical) {
            ZAMIN_LOGE("Grafika+present navbati bor qurilma topilmadi");
            return false;
        }

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(m_physical, &props);
        std::snprintf(m_deviceName, sizeof(m_deviceName), "%s", props.deviceName);
        ZAMIN_LOGI("GPU: %s (Vulkan %u.%u.%u)", props.deviceName,
                   VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion),
                   VK_VERSION_PATCH(props.apiVersion));

        const float priority = 1.0f;
        VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        qci.queueFamilyIndex = m_queueFamily;
        qci.queueCount = 1;
        qci.pQueuePriorities = &priority;

        const char* deviceExt[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        dci.queueCreateInfoCount = 1;
        dci.pQueueCreateInfos = &qci;
        dci.enabledExtensionCount = m_headless ? 0 : 1;
        dci.ppEnabledExtensionNames = deviceExt;
        VK_CHECK(vkCreateDevice(m_physical, &dci, nullptr, &m_device));
        vkGetDeviceQueue(m_device, m_queueFamily, 0, &m_queue);

        VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pci.queueFamilyIndex = m_queueFamily;
        VK_CHECK(vkCreateCommandPool(m_device, &pci, nullptr, &m_cmdPool));

        VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cai.commandPool = m_cmdPool;
        cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cai.commandBufferCount = 1;
        for (auto& f : m_frames) {
            VK_CHECK(vkAllocateCommandBuffers(m_device, &cai, &f.cmd));
            VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            VK_CHECK(vkCreateSemaphore(m_device, &sci, nullptr, &f.imageAvailable));
            VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            VK_CHECK(vkCreateFence(m_device, &fci, nullptr, &f.fence));
        }
        return true;
    }

    bool createSwapchain() {
        VkSurfaceCapabilitiesKHR caps;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical, m_surface, &caps));

        uint32_t fmtCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical, m_surface, &fmtCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(fmtCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical, m_surface, &fmtCount, formats.data());
        if (formats.empty()) return false;
        VkSurfaceFormatKHR chosen = formats[0];
        for (auto& f : formats) {
            if (f.format == VK_FORMAT_R8G8B8A8_SRGB || f.format == VK_FORMAT_B8G8R8A8_SRGB) {
                chosen = f;
                break;
            }
        }
        m_format = chosen.format;

        // Android pre-rotation: displey burilishini compositor emas, biz bajaramiz
        // (ortiqcha GPU nusxalashning oldini oladi).
        m_transform = caps.currentTransform;
        VkExtent2D extent = caps.currentExtent;
        if (extent.width == 0xFFFFFFFFu) extent = {1, 1};
        if (m_transform & (VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR |
                           VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)) {
            std::swap(extent.width, extent.height);
        }
        if (extent.width == 0 || extent.height == 0) return false;
        m_extent = extent;

        uint32_t imageCount = std::max(caps.minImageCount, 3u);
        if (caps.maxImageCount > 0) imageCount = std::min(imageCount, caps.maxImageCount);

        VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (!(caps.supportedCompositeAlpha & alpha)) alpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

        VkSwapchainCreateInfoKHR sci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        sci.surface = m_surface;
        sci.minImageCount = imageCount;
        sci.imageFormat = chosen.format;
        sci.imageColorSpace = chosen.colorSpace;
        sci.imageExtent = m_extent;
        sci.imageArrayLayers = 1;
        sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        sci.preTransform = m_transform;
        sci.compositeAlpha = alpha;
        sci.presentMode = VK_PRESENT_MODE_FIFO_KHR; // V-Sync, batareyani tejaydi
        sci.clipped = VK_TRUE;
        sci.oldSwapchain = VK_NULL_HANDLE;
        VK_CHECK(vkCreateSwapchainKHR(m_device, &sci, nullptr, &m_swapchain));

        uint32_t count = 0;
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, nullptr);
        std::vector<VkImage> images(count);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, images.data());

        if (!createDepth()) return false;
        if (!m_renderPass && !createRenderPass()) return false;
        if (!m_pipeline && !createPipeline()) return false;

        m_images.resize(count);
        for (uint32_t i = 0; i < count; ++i) {
            SwapImage& si = m_images[i];
            si.image = images[i];
            VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            vci.image = si.image;
            vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vci.format = m_format;
            vci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            VK_CHECK(vkCreateImageView(m_device, &vci, nullptr, &si.view));

            VkImageView attachments[] = {si.view, m_depthView};
            VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fci.renderPass = m_renderPass;
            fci.attachmentCount = 2;
            fci.pAttachments = attachments;
            fci.width = m_extent.width;
            fci.height = m_extent.height;
            fci.layers = 1;
            VK_CHECK(vkCreateFramebuffer(m_device, &fci, nullptr, &si.framebuffer));

            VkSemaphoreCreateInfo smci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            VK_CHECK(vkCreateSemaphore(m_device, &smci, nullptr, &si.renderFinished));
        }
        m_needsRecreate = false;
        return true;
    }

    uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags flags) {
        VkPhysicalDeviceMemoryProperties mp;
        vkGetPhysicalDeviceMemoryProperties(m_physical, &mp);
        for (uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
            if ((typeBits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & flags) == flags)
                return i;
        }
        return UINT32_MAX;
    }

    bool createDepth() {
        VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ici.imageType = VK_IMAGE_TYPE_2D;
        ici.format = m_depthFormat;
        ici.extent = {m_extent.width, m_extent.height, 1};
        ici.mipLevels = 1;
        ici.arrayLayers = 1;
        ici.samples = VK_SAMPLE_COUNT_1_BIT;
        ici.tiling = VK_IMAGE_TILING_OPTIMAL;
        // Mobil TBDR GPU'larda depth faqat tile xotirasida yashashi mumkin
        ici.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VK_CHECK(vkCreateImage(m_device, &ici, nullptr, &m_depthImage));

        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(m_device, m_depthImage, &req);
        uint32_t type = findMemoryType(req.memoryTypeBits,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                           VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
        if (type == UINT32_MAX)
            type = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (type == UINT32_MAX) return false;

        VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        mai.allocationSize = req.size;
        mai.memoryTypeIndex = type;
        VK_CHECK(vkAllocateMemory(m_device, &mai, nullptr, &m_depthMemory));
        VK_CHECK(vkBindImageMemory(m_device, m_depthImage, m_depthMemory, 0));

        VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vci.image = m_depthImage;
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format = m_depthFormat;
        vci.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
        VK_CHECK(vkCreateImageView(m_device, &vci, nullptr, &m_depthView));
        return true;
    }

    bool createRenderPass() {
        VkAttachmentDescription att[2]{};
        att[0].format = m_format;
        att[0].samples = VK_SAMPLE_COUNT_1_BIT;
        att[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        att[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        att[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        att[0].finalLayout = m_headless ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                                        : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        att[1].format = m_depthFormat;
        att[1].samples = VK_SAMPLE_COUNT_1_BIT;
        att[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        att[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // tile xotiradan chiqmaydi
        att[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        att[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        VkSubpassDescription sub{};
        sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sub.colorAttachmentCount = 1;
        sub.pColorAttachments = &colorRef;
        sub.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency dep{};
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dep.dstStageMask = dep.srcStageMask;
        dep.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

        // Offscreen: chizish tugagach nusxalashdan oldin kutish
        VkSubpassDependency deps[2] = {dep, {}};
        deps[1].srcSubpass = 0;
        deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        VkRenderPassCreateInfo rci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rci.attachmentCount = 2;
        rci.pAttachments = att;
        rci.subpassCount = 1;
        rci.pSubpasses = &sub;
        rci.dependencyCount = m_headless ? 2 : 1;
        rci.pDependencies = deps;
        VK_CHECK(vkCreateRenderPass(m_device, &rci, nullptr, &m_renderPass));
        return true;
    }

    VkShaderModule createShader(const uint32_t* code, size_t bytes) {
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = bytes;
        ci.pCode = code;
        VkShaderModule mod = VK_NULL_HANDLE;
        if (vkCreateShaderModule(m_device, &ci, nullptr, &mod) != VK_SUCCESS) return VK_NULL_HANDLE;
        return mod;
    }

    bool createPipeline() {
        VkPushConstantRange pcr{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                sizeof(PushConstants)};
        VkPipelineLayoutCreateInfo lci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        lci.pushConstantRangeCount = 1;
        lci.pPushConstantRanges = &pcr;
        VK_CHECK(vkCreatePipelineLayout(m_device, &lci, nullptr, &m_pipelineLayout));

        m_pipeline = buildPipeline(kBasicVertSpv, sizeof(kBasicVertSpv), kBasicFragSpv,
                                   sizeof(kBasicFragSpv), /*transparent=*/false);
        m_gridPipeline = buildPipeline(kGridVertSpv, sizeof(kGridVertSpv), kGridFragSpv,
                                       sizeof(kGridFragSpv), /*transparent=*/true);
        return m_pipeline && m_gridPipeline;
    }

    VkPipeline buildPipeline(const uint32_t* vsCode, size_t vsBytes, const uint32_t* fsCode,
                             size_t fsBytes, bool transparent) {
        VkShaderModule vs = createShader(vsCode, vsBytes);
        VkShaderModule fs = createShader(fsCode, fsBytes);
        if (!vs || !fs) {
            ZAMIN_LOGE("Shader modul yaratilmadi");
            return VK_NULL_HANDLE;
        }

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vs;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fs;
        stages[1].pName = "main";

        VkPipelineVertexInputStateCreateInfo vin{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        vp.viewportCount = 1;
        vp.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_NONE;
        rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        ds.depthTestEnable = VK_TRUE;
        ds.depthWriteEnable = transparent ? VK_FALSE : VK_TRUE;
        ds.depthCompareOp = VK_COMPARE_OP_LESS;

        VkPipelineColorBlendAttachmentState cba{};
        if (transparent) {
            cba.blendEnable = VK_TRUE;
            cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            cba.colorBlendOp = VK_BLEND_OP_ADD;
            cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            cba.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        cb.attachmentCount = 1;
        cb.pAttachments = &cba;

        VkDynamicState dyn[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dsi{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dsi.dynamicStateCount = 2;
        dsi.pDynamicStates = dyn;

        VkGraphicsPipelineCreateInfo pci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pci.stageCount = 2;
        pci.pStages = stages;
        pci.pVertexInputState = &vin;
        pci.pInputAssemblyState = &ia;
        pci.pViewportState = &vp;
        pci.pRasterizationState = &rs;
        pci.pMultisampleState = &ms;
        pci.pDepthStencilState = &ds;
        pci.pColorBlendState = &cb;
        pci.pDynamicState = &dsi;
        pci.layout = m_pipelineLayout;
        pci.renderPass = m_renderPass;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkResult r = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pci, nullptr, &pipeline);
        vkDestroyShaderModule(m_device, vs, nullptr);
        vkDestroyShaderModule(m_device, fs, nullptr);
        if (r != VK_SUCCESS) {
            ZAMIN_LOGE("vkCreateGraphicsPipelines: %d", r);
            return VK_NULL_HANDLE;
        }
        return pipeline;
    }

    void recordCommands(VkCommandBuffer cmd, uint32_t imageIndex, const FrameParams& p) {
        recordScene(cmd, m_images[imageIndex].framebuffer, p, /*end=*/true);
    }

    void recordScene(VkCommandBuffer cmd, VkFramebuffer framebuffer, const FrameParams& p, bool end) {
        vkResetCommandBuffer(cmd, 0);
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &bi);

        const bool srgb = isSrgb(m_format);
        VkClearValue clears[2];
        for (int i = 0; i < 3; ++i)
            clears[0].color.float32[i] = srgb ? srgbToLinear(p.clearColor[i]) : p.clearColor[i];
        clears[0].color.float32[3] = p.clearColor[3];
        clears[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo rbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rbi.renderPass = m_renderPass;
        rbi.framebuffer = framebuffer;
        rbi.renderArea = {{0, 0}, m_extent};
        rbi.clearValueCount = 2;
        rbi.pClearValues = clears;
        vkCmdBeginRenderPass(cmd, &rbi, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{0, 0, float(m_extent.width), float(m_extent.height), 0.0f, 1.0f};
        VkRect2D scissor{{0, 0}, m_extent};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        // Pre-rotation: displey yo'nalishidagi aspect va burilish
        bool swapped = m_transform & (VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR |
                                      VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR);
        float w = float(m_extent.width), h = float(m_extent.height);
        float aspect = swapped ? h / w : w / h;
        float preRot = 0.0f;
        if (m_transform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) preRot = 1.5707963f;
        else if (m_transform & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR) preRot = 3.1415926f;
        else if (m_transform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) preRot = 4.712389f;

        // Orbit kamera: nishon atrofida yaw/pitch, masofa
        Mat4 view = mul(translation(0.0f, -0.1f, -p.cameraDistance),
                        mul(rotationX(p.rotationX), rotationY(p.rotationY)));
        // Portret ekranda gorizontal ko'rish burchagini saqlaymiz (telefonda obyekt juda katta ko'rinmasin)
        const float baseFov = 0.9f;
        const float fovY = aspect >= 1.0f ? baseFov
                                          : 2.0f * std::atan(std::tan(baseFov * 0.5f) / aspect);
        Mat4 proj = perspective(std::fmin(fovY, 1.6f), aspect, 0.05f, 200.0f);
        Mat4 viewProj = mul(rotationZ(preRot), mul(proj, view));

        PushConstants pc;
        pc.params[0] = srgb ? 1.0f : 0.0f;
        pc.params[1] = pc.params[2] = pc.params[3] = 0.0f;
        const VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        // 1) Shaffof bo'lmagan obyektlar
        Mat4 model = rotationY(p.objectSpin);
        Mat4 mvp = mul(viewProj, model);
        std::memcpy(pc.mvp, mvp.m, sizeof(pc.mvp));
        vkCmdPushConstants(cmd, m_pipelineLayout, stages, 0, sizeof(pc), &pc);
        vkCmdDraw(cmd, 18, 1, 0, 0); // piramida: 6 uchburchak (4 yon + 2 asos)

        // 2) Yer to'ri (grid) — shaffof, depth yozmaydi
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gridPipeline);
        std::memcpy(pc.mvp, viewProj.m, sizeof(pc.mvp));
        vkCmdPushConstants(cmd, m_pipelineLayout, stages, 0, sizeof(pc), &pc);
        vkCmdDraw(cmd, 6, 1, 0, 0);

        vkCmdEndRenderPass(cmd);
        if (end) vkEndCommandBuffer(cmd);
    }

    bool recreateSwapchain() {
        vkDeviceWaitIdle(m_device);
        destroySwapchain();
        return createSwapchain();
    }

    void destroySwapchain() {
        if (!m_device) return;
        for (auto& si : m_images) {
            if (si.framebuffer) vkDestroyFramebuffer(m_device, si.framebuffer, nullptr);
            if (si.view) vkDestroyImageView(m_device, si.view, nullptr);
            if (si.renderFinished) vkDestroySemaphore(m_device, si.renderFinished, nullptr);
        }
        m_images.clear();
        if (m_depthView) vkDestroyImageView(m_device, m_depthView, nullptr);
        if (m_depthImage) vkDestroyImage(m_device, m_depthImage, nullptr);
        if (m_depthMemory) vkFreeMemory(m_device, m_depthMemory, nullptr);
        m_depthView = VK_NULL_HANDLE;
        m_depthImage = VK_NULL_HANDLE;
        m_depthMemory = VK_NULL_HANDLE;
        if (m_swapchain) vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }

    void destroySurface() {
        if (m_surface) vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    void destroyDevice() {
        if (m_device) {
            vkDeviceWaitIdle(m_device);
            if (m_pipeline) vkDestroyPipeline(m_device, m_pipeline, nullptr);
            if (m_gridPipeline) vkDestroyPipeline(m_device, m_gridPipeline, nullptr);
            if (m_pipelineLayout) vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            if (m_renderPass) vkDestroyRenderPass(m_device, m_renderPass, nullptr);
            for (auto& f : m_frames) {
                if (f.fence) vkDestroyFence(m_device, f.fence, nullptr);
                if (f.imageAvailable) vkDestroySemaphore(m_device, f.imageAvailable, nullptr);
            }
            if (m_cmdPool) vkDestroyCommandPool(m_device, m_cmdPool, nullptr);
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;
        }
        if (m_instance) {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }
    }

    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physical = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
    uint32_t m_queueFamily = 0;
    VkCommandPool m_cmdPool = VK_NULL_HANDLE;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent{};
    VkSurfaceTransformFlagBitsKHR m_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    std::vector<SwapImage> m_images;

    // D16 barcha Android Vulkan qurilmalarida depth attachment sifatida majburiy
    VkFormat m_depthFormat = VK_FORMAT_D16_UNORM;
    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthMemory = VK_NULL_HANDLE;
    VkImageView m_depthView = VK_NULL_HANDLE;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipeline m_gridPipeline = VK_NULL_HANDLE;

    std::array<Frame, kFramesInFlight> m_frames{};
    uint32_t m_frameIndex = 0;
    bool m_needsRecreate = false;
    bool m_headless = false;
    char m_deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE] = "Unknown";
};

} // namespace

std::unique_ptr<Renderer> createVulkanRenderer() { return std::make_unique<VulkanRenderer>(); }

} // namespace zamin::rhi
