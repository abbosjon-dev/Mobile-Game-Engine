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


// Editor'dagi standart sahna bilan bir xil (EditorState.kt)
void buildDemoScene(zamin::Scene& scene) {
    using zamin::EntityDesc;
    using zamin::MeshKind;
    auto add = [&](uint32_t id, uint32_t parent, MeshKind mesh, zamin::Vec3 pos, zamin::Vec3 rot,
                   zamin::Vec3 scale, uint32_t rgb) {
        EntityDesc e;
        e.id = id;
        e.parent = parent;
        e.mesh = mesh;
        e.position = pos;
        e.rotation = rot;
        e.scale = scale;
        e.color[0] = ((rgb >> 16) & 0xFF) / 255.0f;
        e.color[1] = ((rgb >> 8) & 0xFF) / 255.0f;
        e.color[2] = (rgb & 0xFF) / 255.0f;
        scene.upsert(e);
    };
    add(1, 0, MeshKind::None, {}, {}, {1, 1, 1}, 0xFFFFFF);
    add(2, 1, MeshKind::None, {0, 1.6f, -6}, {}, {1, 1, 1}, 0xFFFFFF);
    add(3, 1, MeshKind::None, {0, 8, 0}, {50, -30, 0}, {1, 1, 1}, 0xFFFFFF);
    add(4, 1, MeshKind::Pyramid, {0, 0.5f, 0}, {}, {1, 1, 1}, 0x3DB98A);
    add(5, 1, MeshKind::Cube, {-1.8f, 0.5f, 0.8f}, {0, 25, 0}, {1, 1, 1}, 0xF2A33C);
    add(6, 1, MeshKind::Sphere, {1.7f, 0.5f, -0.6f}, {}, {1, 1, 1}, 0x4D8CF2);
    add(7, 1, MeshKind::Plane, {0, 0, 0}, {}, {10, 1, 10}, 0x3F4A45);
    add(8, 1, MeshKind::None, {1.2f, 0, 2}, {}, {1, 1, 1}, 0xFFFFFF);
    add(9, 8, MeshKind::Quad, {0, 0.75f, 0}, {}, {1, 1.5f, 1}, 0xE8604C);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr,
                     "Ishlatish: %s <out.png> <width> <height> [yaw] [pitch] [tanlangan_id] [2d]\n",
                     argv[0]);
        return 2;
    }
    const uint32_t w = std::atoi(argv[2]);
    const uint32_t h = std::atoi(argv[3]);
    zamin::rhi::FrameParams params;
    params.camera.yaw = argc > 4 ? std::atof(argv[4]) : 0.6f;
    params.camera.pitch = argc > 5 ? std::atof(argv[5]) : 0.42f;
    const uint32_t selected = argc > 6 ? std::atoi(argv[6]) : 4;
    params.camera.mode2D = argc > 7 && std::atoi(argv[7]) != 0;
    if (params.camera.mode2D) params.camera.target = {0.0f, 0.5f, 0.0f};

    zamin::Scene scene;
    buildDemoScene(scene);
    scene.buildDrawList(params.items, selected);
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
