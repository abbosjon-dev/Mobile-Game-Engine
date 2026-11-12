#pragma once

#include "zamin/math/Math.h"

namespace zamin {

// Editor kamerasi.
// 3D: nishon (target) atrofida aylanuvchi orbit kamera (perspektiv).
// 2D: XY tekisligiga -Z yo'nalishida qaraydigan ortografik kamera.
struct EditorCamera {
    Vec3 target{0.0f, 0.5f, 0.0f};
    float yaw = 0.6f;
    float pitch = 0.45f;
    float distance = 7.0f;
    bool mode2D = false;

    Mat4 view() const {
        if (mode2D) return translation({-target.x, -target.y, -50.0f});
        return mul(translation({0.0f, 0.0f, -distance}),
                   mul(rotationX(pitch), mul(rotationY(yaw), translation(target * -1.0f))));
    }

    // aspect = kenglik / balandlik (displey yo'nalishida)
    Mat4 projection(float aspect) const {
        if (mode2D) {
            const float halfH = aspect >= 1.0f ? distance * 0.5f : distance * 0.5f / aspect;
            return orthographic(halfH * aspect, halfH, 0.1f, 200.0f);
        }
        // Portret ekranda gorizontal ko'rish burchagini saqlaymiz
        const float baseFov = 0.9f;
        const float fovY = aspect >= 1.0f ? baseFov
                                          : 2.0f * std::atan(std::tan(baseFov * 0.5f) / aspect);
        return perspective(std::fmin(fovY, 1.6f), aspect, 0.05f, 300.0f);
    }

    // 2D rejimda bir piksel nechta dunyo birligiga teng
    float worldPerPixel2D(float aspect, float heightPx) const {
        const float halfH = aspect >= 1.0f ? distance * 0.5f : distance * 0.5f / aspect;
        return 2.0f * halfH / heightPx;
    }

    // Ekran nuqtasidan (piksel, yuqori-chap boshlanish) dunyo nuri.
    bool screenRay(float x, float y, float width, float height, Vec3& origin, Vec3& dir) const {
        if (width <= 0.0f || height <= 0.0f) return false;
        Mat4 inv;
        if (!inverse(mul(projection(width / height), view()), inv)) return false;
        const float nx = 2.0f * x / width - 1.0f;
        const float ny = 2.0f * y / height - 1.0f;
        const Vec3 nearP = transformPoint(inv, {nx, ny, 0.0f});
        const Vec3 farP = transformPoint(inv, {nx, ny, 1.0f});
        origin = nearP;
        dir = normalize(farP - nearP);
        return true;
    }
};

} // namespace zamin
