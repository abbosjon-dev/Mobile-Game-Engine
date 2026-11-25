#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "zamin/math/Math.h"

namespace zamin {

// Ichki (primitive) meshlar. Barchasi lokal [-0.5, 0.5] kubiga sig'adi.
// 0..Count oralig'i editor (Kotlin MeshKind) bilan umumiy, undan keyingilari faqat engine ichida.
enum class MeshKind : uint8_t {
    None = 0, // bo'sh obyekt, kamera, yorug'lik — mesh yo'q
    Cube = 1,
    Sphere = 2,
    Pyramid = 3,
    Plane = 4, // XZ tekisligi, normal +Y
    Quad = 5,  // XY tekisligi, normal +Z (2D sprite uchun)
    Count,
    // ---- Editor belgilari (gizmo), yoritilmaydi ----
    GizmoLight = Count, // olmos + yo'nalish tayoqchasi (+Z)
    GizmoCamera,        // korpus + obyektiv (+Z ga qaraydi)
    GizmoEmpty,         // uch o'qli xoch
    InternalCount
};

// Meshsiz obyektlar viewport'da qanday belgi bilan ko'rsatiladi.
// Kotlin'dagi Gizmo enum bilan bir xil tartib.
enum class GizmoKind : uint8_t { None = 0, Light = 1, Camera = 2, Empty = 3, Count };

struct EntityDesc {
    uint32_t id = 0;     // 0 — yaroqsiz
    uint32_t parent = 0; // 0 — ildiz
    MeshKind mesh = MeshKind::None;
    GizmoKind gizmo = GizmoKind::None;
    bool visible = true;
    Vec3 position{};
    Vec3 rotation{}; // gradus
    Vec3 scale{1, 1, 1};
    float color[4] = {1, 1, 1, 1}; // sRGB
};

// Renderer uchun tayyor chizish elementi
struct DrawItem {
    Mat4 model;
    MeshKind mesh = MeshKind::None;
    float color[4] = {1, 1, 1, 1};
    bool selected = false;
    Vec3 scale{1, 1, 1}; // dunyo masshtabi (outline qalinligi uchun)
};

struct DrawOptions {
    uint32_t selectedId = 0;
    bool gizmos = true; // yorug'lik/kamera/bo'sh obyekt belgilari
};

// Sahna: obyektlar va ularning ierarxiyasi.
// Dunyo matritsalari keshlanadi va sahna o'zgarganda qayta hisoblanadi.
class Scene {
public:
    // Gizmo belgisining standart dunyo o'lchami (obyekt masshtabiga bog'liq emas)
    static constexpr float kGizmoSize = 0.45f;
    // Editor gizmolarni ekranda taxminan bir xil ko'rinishi uchun kamera masofasiga qarab o'zgartiradi
    void setGizmoSize(float size) { m_gizmoSize = size > 0.0f ? size : kGizmoSize; }
    float gizmoSize() const { return m_gizmoSize; }

    void upsert(const EntityDesc& e);
    void remove(uint32_t id);
    void clear();
    const EntityDesc* find(uint32_t id) const;
    size_t size() const { return m_entities.size(); }

    Mat4 worldMatrix(uint32_t id) const;
    Mat4 parentMatrix(uint32_t id) const;
    bool effectiveVisible(uint32_t id) const;
    // Gizmo belgisi uchun model matritsa: dunyo pozitsiyasi va aylanishi, doimiy o'lcham
    Mat4 gizmoMatrix(uint32_t id) const;

    void buildDrawList(std::vector<DrawItem>& out, const DrawOptions& opt = {}) const;

    // Nur bilan eng yaqin obyekt (gizmolar ham); 0 — hech narsa.
    uint32_t pick(Vec3 origin, Vec3 dir, bool gizmos = true) const;

    // Obyektni dunyo fazosida siljitish (ota-ona fazosiga o'tkaziladi).
    bool translateWorld(uint32_t id, Vec3 worldDelta);
    // Obyektni dunyo fazosidagi aylanish bilan burish (o'z markazi atrofida).
    bool rotateWorld(uint32_t id, const Mat4& worldRotation);
    Vec3 worldPosition(uint32_t id) const;

    // Birinchi ko'rinadigan kamera obyekti (eng kichik id); 0 — yo'q.
    uint32_t mainCamera() const;
    // Kamera obyektining ko'rinish matritsasi (kamera +Z ga qaraydi).
    bool cameraView(uint32_t id, Mat4& outView) const;

    // Keshni qayta hisoblashlar soni (testlar uchun)
    uint64_t cacheRebuilds() const { return m_rebuilds; }

private:
    struct Cached {
        Mat4 world;
        bool visible = true;
    };
    void rebuildCache() const;
    const Cached* cached(uint32_t id) const;
    void markDirty() { m_dirty = true; }

    std::unordered_map<uint32_t, EntityDesc> m_entities;
    mutable std::unordered_map<uint32_t, Cached> m_cache;
    mutable bool m_dirty = true;
    mutable uint64_t m_rebuilds = 0;
    float m_gizmoSize = kGizmoSize;
};

} // namespace zamin
