#include "zamin/scene/Scene.h"

#include <algorithm>
#include <cfloat>

namespace zamin {

namespace {
constexpr int kMaxDepth = 64; // ierarxiyada sikl bo'lsa ham to'xtaymiz

// Nur va lokal AABB kesishuvi (slab usuli). t — nur parametri.
bool rayAabb(Vec3 o, Vec3 d, Vec3 mn, Vec3 mx, float& tHit) {
    float tmin = -FLT_MAX, tmax = FLT_MAX;
    const float oo[3] = {o.x, o.y, o.z}, dd[3] = {d.x, d.y, d.z};
    const float lo[3] = {mn.x, mn.y, mn.z}, hi[3] = {mx.x, mx.y, mx.z};
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(dd[i]) < 1e-8f) {
            if (oo[i] < lo[i] || oo[i] > hi[i]) return false;
            continue;
        }
        float t1 = (lo[i] - oo[i]) / dd[i];
        float t2 = (hi[i] - oo[i]) / dd[i];
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }
    if (tmax < 0.0f) return false;
    tHit = tmin >= 0.0f ? tmin : tmax;
    return true;
}

// Nur va lokal sfera (markaz 0, r = 0.5)
bool raySphere(Vec3 o, Vec3 d, float& tHit) {
    const float a = dot(d, d);
    const float b = 2.0f * dot(o, d);
    const float c = dot(o, o) - 0.25f;
    const float disc = b * b - 4 * a * c;
    if (disc < 0.0f) return false;
    const float s = std::sqrt(disc);
    float t = (-b - s) / (2 * a);
    if (t < 0.0f) t = (-b + s) / (2 * a);
    if (t < 0.0f) return false;
    tHit = t;
    return true;
}
} // namespace

void Scene::upsert(const EntityDesc& e) {
    if (e.id == 0) return;
    m_entities[e.id] = e;
}

void Scene::remove(uint32_t id) { m_entities.erase(id); }

const EntityDesc* Scene::find(uint32_t id) const {
    auto it = m_entities.find(id);
    return it == m_entities.end() ? nullptr : &it->second;
}

Mat4 Scene::parentMatrix(uint32_t id) const {
    const EntityDesc* e = find(id);
    if (!e || e->parent == 0) return Mat4::identity();
    return worldMatrix(e->parent);
}

Mat4 Scene::worldMatrix(uint32_t id) const {
    Mat4 m = Mat4::identity();
    const EntityDesc* e = find(id);
    for (int depth = 0; e && depth < kMaxDepth; ++depth) {
        m = mul(trs(e->position, e->rotation, e->scale), m);
        e = e->parent ? find(e->parent) : nullptr;
    }
    return m;
}

bool Scene::effectiveVisible(uint32_t id) const {
    const EntityDesc* e = find(id);
    for (int depth = 0; e && depth < kMaxDepth; ++depth) {
        if (!e->visible) return false;
        e = e->parent ? find(e->parent) : nullptr;
    }
    return true;
}

void Scene::buildDrawList(std::vector<DrawItem>& out, uint32_t selectedId) const {
    out.clear();
    for (const auto& [id, e] : m_entities) {
        if (e.mesh == MeshKind::None || e.mesh >= MeshKind::Count) continue;
        if (!effectiveVisible(id)) continue;
        DrawItem item;
        item.model = worldMatrix(id);
        item.mesh = e.mesh;
        std::copy(e.color, e.color + 4, item.color);
        item.selected = id == selectedId;
        const Mat4& m = item.model;
        item.scale = {length({m.m[0], m.m[1], m.m[2]}), length({m.m[4], m.m[5], m.m[6]}),
                      length({m.m[8], m.m[9], m.m[10]})};
        out.push_back(item);
    }
    // Tanlangan obyekt oxirida chiziladi (outline undan oldin chiziladi)
    std::stable_sort(out.begin(), out.end(), [](const DrawItem& a, const DrawItem& b) {
        return a.selected < b.selected;
    });
}

uint32_t Scene::pick(Vec3 origin, Vec3 dir) const {
    uint32_t best = 0;
    float bestDist = FLT_MAX;
    for (const auto& [id, e] : m_entities) {
        if (e.mesh == MeshKind::None || e.mesh >= MeshKind::Count) continue;
        if (!effectiveVisible(id)) continue;
        const Mat4 world = worldMatrix(id);
        Mat4 inv;
        if (!inverse(world, inv)) continue;
        const Vec3 lo = transformPoint(inv, origin);
        const Vec3 ld = transformDir(inv, dir);
        float t = 0.0f;
        bool hit = false;
        switch (e.mesh) {
            case MeshKind::Sphere: hit = raySphere(lo, ld, t); break;
            case MeshKind::Plane: hit = rayAabb(lo, ld, {-0.5f, -0.02f, -0.5f}, {0.5f, 0.02f, 0.5f}, t); break;
            case MeshKind::Quad: hit = rayAabb(lo, ld, {-0.5f, -0.5f, -0.02f}, {0.5f, 0.5f, 0.02f}, t); break;
            default: hit = rayAabb(lo, ld, {-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, t); break;
        }
        if (!hit) continue;
        // Lokal t ni dunyo masofasiga o'tkazamiz (masshtab har xil bo'lishi mumkin)
        const Vec3 worldHit = transformPoint(world, lo + ld * t);
        float dist = length(worldHit - origin);
        // Tekisliklar (yer) boshqa obyektlarga nisbatan kamroq ustuvor
        if (e.mesh == MeshKind::Plane) dist += 0.05f;
        if (dist < bestDist) {
            bestDist = dist;
            best = id;
        }
    }
    return best;
}

Vec3 Scene::worldPosition(uint32_t id) const {
    const Mat4 m = worldMatrix(id);
    return {m.m[12], m.m[13], m.m[14]};
}

bool Scene::translateWorld(uint32_t id, Vec3 worldDelta) {
    auto it = m_entities.find(id);
    if (it == m_entities.end()) return false;
    Mat4 invParent;
    if (!inverse(parentMatrix(id), invParent)) return false;
    const Vec3 local = transformDir(invParent, worldDelta);
    it->second.position = it->second.position + local;
    return true;
}

} // namespace zamin
