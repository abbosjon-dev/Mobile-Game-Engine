// zamin_snapshot — engine kadrini ekransiz (offscreen) chizib PNG ga yozadi.
// Ishlatish: zamin_snapshot <chiqish.png> <kenglik> <balandlik> [rotY] [rotX]
// CI va hujjatlar uchun skrinshot olishda ishlatiladi (lavapipe bilan ham ishlaydi).

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <string>
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


// Editor'dagi standart sahna bilan bir xil (EditorState.kt: defaultSceneObjects)
void buildDemoScene(zamin::Scene& scene) {
    using zamin::EntityDesc;
    using zamin::GizmoKind;
    using zamin::MeshKind;
    auto add = [&](uint32_t id, uint32_t parent, MeshKind mesh, GizmoKind gizmo, zamin::Vec3 pos,
                   zamin::Vec3 rot, zamin::Vec3 scale, uint32_t rgb) {
        EntityDesc e;
        e.id = id;
        e.parent = parent;
        e.mesh = mesh;
        e.gizmo = gizmo;
        e.position = pos;
        e.rotation = rot;
        e.scale = scale;
        e.color[0] = ((rgb >> 16) & 0xFF) / 255.0f;
        e.color[1] = ((rgb >> 8) & 0xFF) / 255.0f;
        e.color[2] = (rgb & 0xFF) / 255.0f;
        scene.upsert(e);
    };
    add(1, 0, MeshKind::None, GizmoKind::None, {}, {}, {1, 1, 1}, 0xFFFFFF);
    add(2, 1, MeshKind::None, GizmoKind::Camera, {0, 2.2f, 6.5f}, {12, 180, 0}, {1, 1, 1}, 0xFFFFFF);
    add(3, 1, MeshKind::None, GizmoKind::Light, {-2.6f, 3.2f, -1.2f}, {50, 30, 0}, {1, 1, 1}, 0xFFFFFF);
    add(4, 1, MeshKind::Pyramid, GizmoKind::Empty, {0, 0.5f, 0}, {}, {1, 1, 1}, 0x3DB98A);
    add(5, 1, MeshKind::Cube, GizmoKind::Empty, {-1.8f, 0.5f, 0.8f}, {0, 25, 0}, {1, 1, 1}, 0xF2A33C);
    add(6, 1, MeshKind::Sphere, GizmoKind::Empty, {1.7f, 0.5f, -0.6f}, {}, {1, 1, 1}, 0x4D8CF2);
    add(7, 1, MeshKind::Plane, GizmoKind::Empty, {0, 0, 0}, {}, {10, 1, 10}, 0x3F4A45);
    add(8, 1, MeshKind::None, GizmoKind::Empty, {1.2f, 0, 2}, {}, {1, 1, 1}, 0xFFFFFF);
    add(9, 8, MeshKind::Quad, GizmoKind::Empty, {0, 0.75f, 0}, {}, {1, 1.5f, 1}, 0xE8604C);
}

} // namespace

// Stsenariylar:
//   editor  — editor ko'rinishi, piramida tanlangan, gizmolar ko'rinadi (standart)
//   light   — yorug'lik gizmosi tanlangan, kamera va boshqa gizmolar ham ko'rinadi
//   rotated — kub dunyo fazosida ikki o'q bo'yicha burilgan (Rotate asbobi natijasi)
//   play    — o'yin rejimi: "Asosiy kamera" ko'rinishi, to'r/gizmo/tanlov yo'q
//   2d      — ortografik 2D ko'rinish
int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr, "Ishlatish: %s <out.png> <width> <height> [editor|light|rotated|play|2d]\n", argv[0]);
        return 2;
    }
    const uint32_t w = std::atoi(argv[2]);
    const uint32_t h = std::atoi(argv[3]);
    const std::string scenario = argc > 4 ? argv[4] : "editor";

    zamin::rhi::FrameParams params;
    params.camera.yaw = 0.6f;
    params.camera.pitch = 0.42f;
    zamin::Scene scene;
    buildDemoScene(scene);
    zamin::DrawOptions opt;
    opt.selectedId = 4;
    auto gizmoSize = [&] { scene.setGizmoSize(std::fmax(0.25f, std::fmin(4.0f, params.camera.distance * 0.09f))); };

    if (scenario == "light") {
        opt.selectedId = 3;
        // Uzoqroqdan: yorug'lik, kamera va bo'sh obyekt belgilari birga ko'rinadi
        params.camera.target = {-0.4f, 1.0f, 2.2f};
        params.camera.yaw = -1.05f;
        params.camera.pitch = 0.38f;
        params.camera.distance = 11.0f;
    } else if (scenario == "rotated") {
        opt.selectedId = 5;
        // Rotate asbobi: gorizontal 60 px va vertikal 90 px surish (Engine::rotateEntity bilan bir xil)
        const float k = 0.01f;
        const zamin::Vec3 right{std::cos(params.camera.yaw), 0.0f, std::sin(params.camera.yaw)};
        scene.rotateWorld(5, zamin::mul(zamin::rotationAxis(right, 90 * k), zamin::rotationY(60 * k)));
        const zamin::Vec3 r = scene.find(5)->rotation;
        ZAMIN_LOGI("Kub aylanishi: (%.1f, %.1f, %.1f)", r.x, r.y, r.z);
    } else if (scenario == "play") {
        opt.selectedId = 0;
        opt.gizmos = false;
        params.showGrid = false;
        zamin::Mat4 view;
        if (scene.cameraView(scene.mainCamera(), view)) {
            params.camera.hasViewOverride = true;
            params.camera.viewOverride = view;
        }
    } else if (scenario == "2d") {
        params.camera.mode2D = true;
        params.camera.target = {0.0f, 0.5f, 0.0f};
        opt.selectedId = 9;
    } else if (scenario != "editor") {
        std::fprintf(stderr, "Noma'lum stsenariy: %s\n", scenario.c_str());
        return 2;
    }

    gizmoSize();
    scene.buildDrawList(params.items, opt);
    auto renderer = zamin::rhi::createVulkanRenderer();
    std::vector<uint8_t> pixels;
    if (!renderer->renderOffscreen(w, h, params, pixels)) {
        ZAMIN_LOGE("Offscreen chizish muvaffaqiyatsiz");
        return 1;
    }
    ZAMIN_LOGI("GPU: %s, draw call: %u", renderer->deviceName(), renderer->drawCalls());
    if (!writePng(argv[1], w, h, pixels)) return 1;
    ZAMIN_LOGI("Yozildi: %s (%ux%u, %s)", argv[1], w, h, scenario.c_str());
    return 0;
}
