// zamin_snapshot — engine kadrini ekransiz (offscreen) chizib PNG ga yozadi.
// Ishlatish: zamin_snapshot <chiqish.png> <kenglik> <balandlik> [rotY] [rotX]
// CI va hujjatlar uchun skrinshot olishda ishlatiladi (lavapipe bilan ham ishlaydi).

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include <zlib.h>

#include "zamin/core/Log.h"
#include "zamin/rhi/Renderer.h"

namespace {

uint32_t crc32(const uint8_t* data, size_t len, uint32_t crc = 0) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        init = true;
    }
    crc = ~crc;
    for (size_t i = 0; i < len; ++i) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

void put32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(x >> 24); v.push_back(x >> 16); v.push_back(x >> 8); v.push_back(x);
}

void chunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& data) {
    put32(out, static_cast<uint32_t>(data.size()));
    std::vector<uint8_t> td(type, type + 4);
    td.insert(td.end(), data.begin(), data.end());
    out.insert(out.end(), td.begin(), td.end());
    put32(out, crc32(td.data(), td.size()));
}

// Oddiy PNG yozuvchi (zlib bilan siqiladi).
bool writePng(const char* path, uint32_t w, uint32_t h, const std::vector<uint8_t>& rgba) {
    std::vector<uint8_t> raw;
    raw.reserve((w * 4 + 1) * h);
    for (uint32_t y = 0; y < h; ++y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgba.begin() + y * w * 4, rgba.begin() + (y + 1) * w * 4);
    }
    uLongf zLen = compressBound(static_cast<uLong>(raw.size()));
    std::vector<uint8_t> z(zLen);
    if (compress2(z.data(), &zLen, raw.data(), static_cast<uLong>(raw.size()), 9) != Z_OK) return false;
    z.resize(zLen);

    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    std::vector<uint8_t> ihdr;
    put32(ihdr, w); put32(ihdr, h);
    ihdr.insert(ihdr.end(), {8, 6, 0, 0, 0}); // 8-bit RGBA
    chunk(png, "IHDR", ihdr);
    chunk(png, "IDAT", z);
    chunk(png, "IEND", {});

    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    std::fwrite(png.data(), 1, png.size(), f);
    std::fclose(f);
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr, "Ishlatish: %s <out.png> <width> <height> [rotY] [rotX]\n", argv[0]);
        return 2;
    }
    const uint32_t w = std::atoi(argv[2]);
    const uint32_t h = std::atoi(argv[3]);
    zamin::rhi::FrameParams params;
    params.rotationY = argc > 4 ? std::atof(argv[4]) : 0.6f;
    params.rotationX = argc > 5 ? std::atof(argv[5]) : 0.35f;

    auto renderer = zamin::rhi::createVulkanRenderer();
    std::vector<uint8_t> pixels;
    if (!renderer->renderOffscreen(w, h, params, pixels)) {
        ZAMIN_LOGE("Offscreen chizish muvaffaqiyatsiz");
        return 1;
    }
    ZAMIN_LOGI("GPU: %s", renderer->deviceName());
    if (!writePng(argv[1], w, h, pixels)) return 1;
    ZAMIN_LOGI("Yozildi: %s (%ux%u)", argv[1], w, h);
    return 0;
}
