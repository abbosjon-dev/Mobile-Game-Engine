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
    markDirty();
}

void Scene::remove(uint32_t id) {
    m_entities.erase(id);
    markDirty();
}

void Scene::clear() {
    m_entities.clear();
    markDirty();
}

const EntityDesc* Scene::find(uint32_t id) const {
    auto it = m_entities.find(id);
    return it == m_entities.end() ? nullptr : &it->second;
}

void Scene::rebuildCache() const {
    m_cache.clear();
    m_cache.reserve(m_entities.size());
    // Ota-onadan boshlab rekursiv hisoblash; har bir obyekt bir marta hisoblanadi.
    // Ierarxiyada sikl bo'lsa kMaxDepth da to'xtaymiz.
    struct Builder {
        const Scene& s;
        const Cached* get(uint32_t id, int depth) {
            if (auto it = s.m_cache.find(id); it != s.m_cache.end()) return &it->second;
            const EntityDesc* e = s.find(id);
            if (!e) return nullptr;
            Cached c;
            c.world = trs(e->position, e->rotation, e->scale);
            c.visible = e->visible;
            if (e->parent != 0 && e->parent != id && depth < kMaxDepth) {
                if (const Cached* p = get(e->parent, depth + 1)) {
                    c.world = mul(p->world, c.world);
                    c.visible = c.visible && p->visible;
                }
            }
            return &s.m_cache.emplace(id, c).first->second;
        }
    } b{*this};
    for (const auto& [id, e] : m_entities) b.get(id, 0);
    m_dirty = false;
    ++m_rebuilds;
}

const Scene::Cached* Scene::cached(uint32_t id) const {
    if (m_dirty) rebuildCache();
    auto it = m_cache.find(id);
    return it == m_cache.end() ? nullptr : &it->second;
}

Mat4 Scene::parentMatrix(uint32_t id) const {
    const EntityDesc* e = find(id);
    if (!e || e->parent == 0) return Mat4::identity();
    return worldMatrix(e->parent);
}

Mat4 Scene::worldMatrix(uint32_t id) const {
    const Cached* c = cached(id);
    return c ? c->world : Mat4::identity();
}

bool Scene::effectiveVisible(uint32_t id) const {
    const Cached* c = cached(id);
    return c && c->visible;
}

namespace {
MeshKind gizmoMesh(GizmoKind g) {
    switch (g) {
        case GizmoKind::Light: return MeshKind::GizmoLight;
        case GizmoKind::Camera: return MeshKind::GizmoCamera;
        case GizmoKind::Empty: return MeshKind::GizmoEmpty;
        default: return MeshKind::None;
    }
}

bool drawsMesh(const EntityDesc& e) { return e.mesh != MeshKind::None && e.mesh < MeshKind::Count; }
bool drawsGizmo(const EntityDesc& e) {
    return !drawsMesh(e) && e.gizmo != GizmoKind::None && e.gizmo < GizmoKind::Count;
}

// Gizmo ranglari (sRGB)
void gizmoColor(GizmoKind g, float out[4]) {
    static const float light[4] = {0.98f, 0.82f, 0.30f, 1.0f};
    static const float camera[4] = {0.78f, 0.82f, 0.88f, 1.0f};
    static const float empty[4] = {0.62f, 0.66f, 0.72f, 1.0f};
    const float* c = g == GizmoKind::Light ? light : g == GizmoKind::Camera ? camera : empty;
    std::copy(c, c + 4, out);
}
} // namespace

Mat4 Scene::gizmoMatrix(uint32_t id) const {
    const Mat4 w = worldMatrix(id);
    Mat4 m = rotationPart(w);
    m.m[12] = w.m[12];
    m.m[13] = w.m[13];
    m.m[14] = w.m[14];
    return mul(m, scaling({m_gizmoSize, m_gizmoSize, m_gizmoSize}));
}

void Scene::buildDrawList(std::vector<DrawItem>& out, const DrawOptions& opt) const {
    out.clear();
    if (m_dirty) rebuildCache();
    for (const auto& [id, e] : m_entities) {
        const bool mesh = drawsMesh(e);
        const bool gizmo = opt.gizmos && drawsGizmo(e);
        if (!mesh && !gizmo) continue;
        const Cached& c = m_cache.at(id);
        if (!c.visible) continue;
        DrawItem item;
        if (mesh) {
            item.model = c.world;
            item.mesh = e.mesh;
            std::copy(e.color, e.color + 4, item.color);
        } else {
            item.model = gizmoMatrix(id);
            item.mesh = gizmoMesh(e.gizmo);
            gizmoColor(e.gizmo, item.color);
        }
        item.selected = id == opt.selectedId;
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

uint32_t Scene::pick(Vec3 origin, Vec3 dir, bool gizmos) const {
    if (m_dirty) rebuildCache();
    uint32_t best = 0;
    float bestDist = FLT_MAX;
    for (const auto& [id, e] : m_entities) {
        const bool mesh = drawsMesh(e);
        const bool gizmo = gizmos && drawsGizmo(e);
        if (!mesh && !gizmo) continue;
        const Cached& c = m_cache.at(id);
        if (!c.visible) continue;
        const Mat4 world = mesh ? c.world : gizmoMatrix(id);
        Mat4 inv;
        if (!inverse(world, inv)) continue;
        const Vec3 lo = transformPoint(inv, origin);
        const Vec3 ld = transformDir(inv, dir);
        float t = 0.0f;
        bool hit = false;
        if (gizmo) {
            hit = raySphere(lo, ld, t); // gizmo belgisini o'rab turuvchi sfera
        } else {
            switch (e.mesh) {
                case MeshKind::Sphere: hit = raySphere(lo, ld, t); break;
                case MeshKind::Plane: hit = rayAabb(lo, ld, {-0.5f, -0.02f, -0.5f}, {0.5f, 0.02f, 0.5f}, t); break;
                case MeshKind::Quad: hit = rayAabb(lo, ld, {-0.5f, -0.5f, -0.02f}, {0.5f, 0.5f, 0.02f}, t); break;
                default: hit = rayAabb(lo, ld, {-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, t); break;
            }
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
    markDirty();
    return true;
}

bool Scene::rotateWorld(uint32_t id, const Mat4& worldRotation) {
    auto it = m_entities.find(id);
    if (it == m_entities.end()) return false;
    const Mat4 parentRot = rotationPart(parentMatrix(id));
    Mat4 invParentRot;
    if (!inverse(parentRot, invParentRot)) return false;
    const Mat4 worldRot = mul(parentRot, rotationEuler(it->second.rotation));
    const Mat4 local = mul(invParentRot, mul(worldRotation, worldRot));
    it->second.rotation = eulerFromRotation(rotationPart(local));
    markDirty();
    return true;
}

uint32_t Scene::mainCamera() const {
    uint32_t best = 0;
    for (const auto& [id, e] : m_entities) {
        if (e.gizmo != GizmoKind::Camera || !effectiveVisible(id)) continue;
        if (best == 0 || id < best) best = id;
    }
    return best;
}

bool Scene::cameraView(uint32_t id, Mat4& outView) const {
    const EntityDesc* e = find(id);
    if (!e) return false;
    const Mat4 w = worldMatrix(id);
    Mat4 m = rotationPart(w);
    m.m[12] = w.m[12];
    m.m[13] = w.m[13];
    m.m[14] = w.m[14];
    // Vulkan ko'rinish fazosida kamera -Z ga qaraydi, bizning kamera obyekti esa +Z ga.
    return inverse(mul(m, rotationY(kPi)), outView);
}

} // namespace zamin
