#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "zamin/math/Math.h"

namespace zamin {

// Ichki (primitive) meshlar. Barchasi lokal [-0.5, 0.5] kubiga sig'adi.
enum class MeshKind : uint8_t {
    None = 0, // bo'sh obyekt, kamera, yorug'lik — chizilmaydi
    Cube = 1,
    Sphere = 2,
    Pyramid = 3,
    Plane = 4, // XZ tekisligi, normal +Y
    Quad = 5,  // XY tekisligi, normal +Z (2D sprite uchun)
    Count
};

struct EntityDesc {
    uint32_t id = 0;     // 0 — yaroqsiz
    uint32_t parent = 0; // 0 — ildiz
    MeshKind mesh = MeshKind::None;
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

// Sahna: obyektlar va ularning ierarxiyasi.
class Scene {
public:
    void upsert(const EntityDesc& e);
    void remove(uint32_t id);
    void clear() { m_entities.clear(); }
    const EntityDesc* find(uint32_t id) const;
    size_t size() const { return m_entities.size(); }

    Mat4 worldMatrix(uint32_t id) const;
    Mat4 parentMatrix(uint32_t id) const;
    bool effectiveVisible(uint32_t id) const;

    void buildDrawList(std::vector<DrawItem>& out, uint32_t selectedId) const;

    // Nur bilan eng yaqin chiziladigan obyekt; 0 — hech narsa.
    uint32_t pick(Vec3 origin, Vec3 dir) const;

    // Obyektni dunyo fazosida siljitish (ota-ona fazosiga o'tkaziladi).
    bool translateWorld(uint32_t id, Vec3 worldDelta);
    Vec3 worldPosition(uint32_t id) const;

private:
    std::unordered_map<uint32_t, EntityDesc> m_entities;
};

} // namespace zamin
