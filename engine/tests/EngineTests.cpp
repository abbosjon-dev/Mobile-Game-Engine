// Zamin engine unit testlari (bog'liqliksiz, oddiy CHECK makrosi bilan).
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "zamin/math/Math.h"
#include "zamin/scene/Camera.h"
#include "zamin/scene/Scene.h"

using namespace zamin;

namespace {
int g_failed = 0;
int g_checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failed;                                                          \
            std::printf("  XATO %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
        }                                                                        \
    } while (0)

bool near(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) <= eps; }
bool near(Vec3 a, Vec3 b, float eps = 1e-3f) {
    return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.z, b.z, eps);
}
bool nearMat(const Mat4& a, const Mat4& b, float eps = 1e-3f) {
    for (int i = 0; i < 16; ++i)
        if (!near(a.m[i], b.m[i], eps)) return false;
    return true;
}

EntityDesc entity(uint32_t id, uint32_t parent, MeshKind mesh, Vec3 pos, GizmoKind gizmo = GizmoKind::None) {
    EntityDesc e;
    e.id = id;
    e.parent = parent;
    e.mesh = mesh;
    e.gizmo = gizmo;
    e.position = pos;
    return e;
}

// ---------------------------------------------------------------- testlar

void testEulerRoundTrip() {
    const Vec3 cases[] = {{0, 0, 0}, {30, 0, 0}, {0, 45, 0}, {0, 0, 60}, {20, -130, 75},
                          {-60, 170, -20}, {89, 10, 0}, {-45, -90, 33}};
    for (Vec3 e : cases) {
        const Mat4 r = rotationEuler(e);
        const Vec3 back = eulerFromRotation(r);
        // Burchaklar boshqacha bo'lishi mumkin, lekin bir xil aylanishni berishi shart
        CHECK(nearMat(rotationEuler(back), r));
    }
    // Gimbal lock (x = ±90) ham to'g'ri aylanishni qaytaradi
    for (Vec3 e : {Vec3{90, 30, 0}, Vec3{-90, -50, 0}, Vec3{90, 20, 40}}) {
        const Mat4 r = rotationEuler(e);
        CHECK(nearMat(rotationEuler(eulerFromRotation(r)), r));
    }
}

void testRotationAxis() {
    // Y o'qi atrofida — rotationY bilan bir xil
    CHECK(nearMat(rotationAxis({0, 1, 0}, 0.7f), rotationY(0.7f)));
    CHECK(nearMat(rotationAxis({1, 0, 0}, -0.4f), rotationX(-0.4f)));
    CHECK(nearMat(rotationAxis({0, 0, 1}, 1.2f), rotationZ(1.2f)));
}

void testFovMatchesProjection() {
    // Portret ekranda pan uchun piksel o'lchami proyeksiya bilan mos bo'lishi kerak:
    // nishon masofasida ekran balandligi = heightPx * worldPerPixel3D.
    EditorCamera cam;
    cam.target = {0, 0, 0};
    cam.yaw = 0;
    cam.pitch = 0;
    cam.distance = 10;
    for (float aspect : {0.5f, 1.0f, 1.8f}) {
        const float h = 1000.0f, w = h * aspect;
        Vec3 o0, d0, o1, d1;
        CHECK(cam.screenRay(w / 2, 0, w, h, o0, d0));
        CHECK(cam.screenRay(w / 2, h, w, h, o1, d1));
        // Nurlarni nishon tekisligi bilan (z = 0) kesishtiramiz
        const Vec3 p0 = o0 + d0 * (-o0.z / d0.z);
        const Vec3 p1 = o1 + d1 * (-o1.z / d1.z);
        const float worldHeight = length(p1 - p0);
        CHECK(near(worldHeight, h * cam.worldPerPixel3D(aspect, h), 0.01f));
    }
}

void testHierarchyAndCache() {
    Scene s;
    s.upsert(entity(1, 0, MeshKind::None, {1, 0, 0}));
    s.upsert(entity(2, 1, MeshKind::Cube, {0, 2, 0}));
    s.upsert(entity(3, 2, MeshKind::Sphere, {0, 0, 3}));
    CHECK(near(s.worldPosition(3), {1, 2, 3}));

    // Kesh: o'zgarishsiz qayta so'rovlar qayta hisoblamaydi
    const uint64_t before = s.cacheRebuilds();
    for (int i = 0; i < 10; ++i) (void)s.worldPosition(3);
    std::vector<DrawItem> items;
    s.buildDrawList(items);
    CHECK(s.cacheRebuilds() == before);

    // Ota-ona o'zgarsa bolalar ham yangilanadi
    s.upsert(entity(1, 0, MeshKind::None, {5, 0, 0}));
    CHECK(near(s.worldPosition(3), {5, 2, 3}));
    CHECK(s.cacheRebuilds() == before + 1);

    // Ko'rinmas ota-ona — bolalari ham chizilmaydi
    EntityDesc hidden = entity(2, 1, MeshKind::Cube, {0, 2, 0});
    hidden.visible = false;
    s.upsert(hidden);
    s.buildDrawList(items);
    CHECK(items.empty());

    // Sikl (1 -> 2 -> 1) bo'lsa ham osilib qolmaydi
    s.upsert(entity(1, 2, MeshKind::None, {0, 0, 0}));
    (void)s.worldPosition(3);
    CHECK(true);
}

void testGizmosDrawAndPick() {
    Scene s;
    s.upsert(entity(10, 0, MeshKind::None, {0, 0, 0}, GizmoKind::Light));
    s.upsert(entity(11, 0, MeshKind::None, {3, 0, 0}, GizmoKind::Camera));
    s.upsert(entity(12, 0, MeshKind::None, {-3, 0, 0}, GizmoKind::Empty));
    s.upsert(entity(13, 0, MeshKind::Cube, {0, 0, -5}, GizmoKind::Empty)); // mesh bor — gizmo emas
    s.upsert(entity(14, 0, MeshKind::None, {6, 0, 0}));                    // na mesh, na gizmo

    std::vector<DrawItem> items;
    s.buildDrawList(items, {.selectedId = 11, .gizmos = true});
    CHECK(items.size() == 4);
    int gizmos = 0;
    for (const DrawItem& it : items) gizmos += it.mesh >= MeshKind::Count;
    CHECK(gizmos == 3);
    CHECK(items.back().selected && items.back().mesh == MeshKind::GizmoCamera);

    // O'yin rejimida gizmolar chizilmaydi
    s.buildDrawList(items, {.selectedId = 0, .gizmos = false});
    CHECK(items.size() == 1 && items[0].mesh == MeshKind::Cube);

    // Gizmo masshtabi obyekt masshtabiga bog'liq emas
    EntityDesc big = entity(10, 0, MeshKind::None, {0, 0, 0}, GizmoKind::Light);
    big.scale = {20, 20, 20};
    s.upsert(big);
    const Mat4 g = s.gizmoMatrix(10);
    CHECK(near(length({g.m[0], g.m[1], g.m[2]}), Scene::kGizmoSize));
    s.setGizmoSize(2.0f);
    const Mat4 g2 = s.gizmoMatrix(10);
    CHECK(near(length({g2.m[0], g2.m[1], g2.m[2]}), 2.0f));
    s.setGizmoSize(Scene::kGizmoSize);

    // Tanlash: yuqoridan pastga nur
    CHECK(s.pick({3, 10, 0}, {0, -1, 0}) == 11);
    CHECK(s.pick({-3, 10, 0}, {0, -1, 0}) == 12);
    CHECK(s.pick({0, 10, 0}, {0, -1, 0}) == 10);
    CHECK(s.pick({3, 10, 0}, {0, -1, 0}, /*gizmos=*/false) == 0);
    CHECK(s.pick({6, 10, 0}, {0, -1, 0}) == 0);
}

void testRotateWorld() {
    Scene s;
    EntityDesc parent = entity(1, 0, MeshKind::None, {0, 0, 0});
    parent.rotation = {0, 90, 0};
    s.upsert(parent);
    s.upsert(entity(2, 1, MeshKind::Cube, {1, 0, 0}));

    const Vec3 posBefore = s.worldPosition(2);
    // Dunyo X o'qi atrofida 90° burish
    CHECK(s.rotateWorld(2, rotationX(kPi / 2)));
    // Pozitsiya o'zgarmaydi (o'z markazi atrofida)
    CHECK(near(s.worldPosition(2), posBefore));
    // Natijaviy dunyo aylanishi = Rx(90) * Ry(90)
    const Mat4 expected = mul(rotationX(kPi / 2), rotationY(kPi / 2));
    CHECK(nearMat(rotationPart(s.worldMatrix(2)), expected));
}

void testCameraView() {
    Scene s;
    s.upsert(entity(1, 0, MeshKind::Cube, {0, 0, 5}));
    EntityDesc cam = entity(2, 0, MeshKind::None, {0, 0, 0}, GizmoKind::Camera);
    s.upsert(cam);
    s.upsert(entity(3, 0, MeshKind::None, {9, 9, 9}, GizmoKind::Camera));
    CHECK(s.mainCamera() == 2);
    Mat4 view;
    CHECK(s.cameraView(2, view));
    // Kamera +Z ga qaraydi: (0,0,5) nuqta ko'rinish fazosida -Z da (oldinda)
    const Vec3 p = transformPoint(view, {0, 0, 5});
    CHECK(near(p, {0, 0, -5}));
    // Ko'rinmas kamera asosiy bo'la olmaydi
    cam.visible = false;
    s.upsert(cam);
    CHECK(s.mainCamera() == 3);
}

void testTranslateWithParent() {
    Scene s;
    EntityDesc parent = entity(1, 0, MeshKind::None, {0, 0, 0});
    parent.scale = {2, 2, 2};
    s.upsert(parent);
    s.upsert(entity(2, 1, MeshKind::Cube, {1, 0, 0}));
    CHECK(s.translateWorld(2, {2, 0, 0}));
    CHECK(near(s.worldPosition(2), {4, 0, 0}));
    CHECK(near(s.find(2)->position, {2, 0, 0}));
}

} // namespace

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"Euler <-> matritsa", testEulerRoundTrip},
        {"Ixtiyoriy o'q atrofida aylanish", testRotationAxis},
        {"Pan: fovY proyeksiya bilan mos", testFovMatchesProjection},
        {"Ierarxiya va dunyo matritsa keshi", testHierarchyAndCache},
        {"Gizmo: chizish va tanlash", testGizmosDrawAndPick},
        {"Dunyo fazosida aylantirish", testRotateWorld},
        {"O'yin kamerasi ko'rinishi", testCameraView},
        {"Ota-ona ichida siljitish", testTranslateWithParent},
    };
    for (const auto& [name, fn] : tests) {
        const int before = g_failed;
        fn();
        std::printf("[%s] %s\n", g_failed == before ? " OK " : "XATO", name);
    }
    std::printf("\n%d tekshiruv, %d xato\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
