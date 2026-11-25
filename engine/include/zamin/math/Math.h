#pragma once
// Zamin — kichik matematik kutubxona (column-major matritsalar, Vulkan clip space).

#include <cmath>

namespace zamin {

constexpr float kPi = 3.14159265358979f;
constexpr float kDegToRad = kPi / 180.0f;

struct Vec3 {
    float x = 0, y = 0, z = 0;
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float length(Vec3 a) { return std::sqrt(dot(a, a)); }
inline Vec3 normalize(Vec3 a) {
    const float l = length(a);
    return l > 0.0f ? a * (1.0f / l) : a;
}

struct Mat4 {
    float m[16] = {};

    static Mat4 identity() {
        Mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }
};

inline Mat4 mul(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int c = 0; c < 4; ++c)
        for (int rr = 0; rr < 4; ++rr) {
            float s = 0.0f;
            for (int k = 0; k < 4; ++k) s += a.m[k * 4 + rr] * b.m[c * 4 + k];
            r.m[c * 4 + rr] = s;
        }
    return r;
}

inline Mat4 rotationX(float a) {
    Mat4 r = Mat4::identity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[5] = c; r.m[6] = s; r.m[9] = -s; r.m[10] = c;
    return r;
}

inline Mat4 rotationY(float a) {
    Mat4 r = Mat4::identity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[0] = c; r.m[2] = -s; r.m[8] = s; r.m[10] = c;
    return r;
}

inline Mat4 rotationZ(float a) {
    Mat4 r = Mat4::identity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[0] = c; r.m[1] = s; r.m[4] = -s; r.m[5] = c;
    return r;
}

inline Mat4 translation(Vec3 t) {
    Mat4 r = Mat4::identity();
    r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
    return r;
}

inline Mat4 scaling(Vec3 s) {
    Mat4 r;
    r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z; r.m[15] = 1.0f;
    return r;
}

// Euler burchaklar (gradus), Unity tartibi: avval Z, keyin X, keyin Y.
inline Mat4 rotationEuler(Vec3 deg) {
    return mul(rotationY(deg.y * kDegToRad),
               mul(rotationX(deg.x * kDegToRad), rotationZ(deg.z * kDegToRad)));
}

inline Mat4 trs(Vec3 pos, Vec3 rotDeg, Vec3 scale) {
    return mul(translation(pos), mul(rotationEuler(rotDeg), scaling(scale)));
}

inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// Ixtiyoriy o'q atrofida aylanish (Rodrig formulasi). axis — birlik vektor, a — radian.
inline Mat4 rotationAxis(Vec3 axis, float a) {
    const float c = std::cos(a), s = std::sin(a), t = 1.0f - c;
    const float x = axis.x, y = axis.y, z = axis.z;
    Mat4 r = Mat4::identity();
    r.m[0] = t * x * x + c;     r.m[4] = t * x * y - s * z; r.m[8] = t * x * z + s * y;
    r.m[1] = t * x * y + s * z; r.m[5] = t * y * y + c;     r.m[9] = t * y * z - s * x;
    r.m[2] = t * x * z - s * y; r.m[6] = t * y * z + s * x; r.m[10] = t * z * z + c;
    return r;
}

// Matritsadan faqat aylanish qismi (ustunlar normallanadi, ko'chirish olib tashlanadi).
inline Mat4 rotationPart(const Mat4& a) {
    Mat4 r = Mat4::identity();
    for (int c = 0; c < 3; ++c) {
        const Vec3 col = normalize({a.m[c * 4], a.m[c * 4 + 1], a.m[c * 4 + 2]});
        r.m[c * 4] = col.x; r.m[c * 4 + 1] = col.y; r.m[c * 4 + 2] = col.z;
    }
    return r;
}

// rotationEuler ga teskari: R = Ry * Rx * Rz dan Euler burchaklar (gradus).
inline Vec3 eulerFromRotation(const Mat4& r) {
    const float sx = -r.m[9]; // R[1][2] = -sin(x)
    const float x = std::asin(std::fmax(-1.0f, std::fmin(1.0f, sx)));
    float y, z;
    if (std::fabs(sx) < 0.9999f) {
        y = std::atan2(r.m[8], r.m[10]); // R[0][2], R[2][2]
        z = std::atan2(r.m[1], r.m[5]);  // R[1][0], R[1][1]
    } else {
        // Gimbal lock: z = 0 deb olamiz
        y = std::atan2(-r.m[2], r.m[0]);
        z = 0.0f;
    }
    return {x / kDegToRad, y / kDegToRad, z / kDegToRad};
}

// Right-handed, depth 0..1, Vulkan Y pastga
inline Mat4 perspective(float fovY, float aspect, float zNear, float zFar) {
    Mat4 r;
    const float f = 1.0f / std::tan(fovY * 0.5f);
    r.m[0] = f / aspect;
    r.m[5] = -f;
    r.m[10] = zFar / (zNear - zFar);
    r.m[11] = -1.0f;
    r.m[14] = (zNear * zFar) / (zNear - zFar);
    return r;
}

inline Mat4 orthographic(float halfW, float halfH, float zNear, float zFar) {
    Mat4 r;
    r.m[0] = 1.0f / halfW;
    r.m[5] = -1.0f / halfH;
    r.m[10] = -1.0f / (zFar - zNear);
    r.m[14] = -zNear / (zFar - zNear);
    r.m[15] = 1.0f;
    return r;
}

inline Vec3 transformPoint(const Mat4& a, Vec3 p) {
    const float x = a.m[0] * p.x + a.m[4] * p.y + a.m[8] * p.z + a.m[12];
    const float y = a.m[1] * p.x + a.m[5] * p.y + a.m[9] * p.z + a.m[13];
    const float z = a.m[2] * p.x + a.m[6] * p.y + a.m[10] * p.z + a.m[14];
    const float w = a.m[3] * p.x + a.m[7] * p.y + a.m[11] * p.z + a.m[15];
    const float iw = w != 0.0f ? 1.0f / w : 1.0f;
    return {x * iw, y * iw, z * iw};
}

inline Vec3 transformDir(const Mat4& a, Vec3 d) {
    return {a.m[0] * d.x + a.m[4] * d.y + a.m[8] * d.z,
            a.m[1] * d.x + a.m[5] * d.y + a.m[9] * d.z,
            a.m[2] * d.x + a.m[6] * d.y + a.m[10] * d.z};
}

// Umumiy 4x4 teskari matritsa (Cramer/kofaktor usuli).
inline bool inverse(const Mat4& in, Mat4& out) {
    const float* m = in.m;
    float inv[16];
    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (std::fabs(det) < 1e-12f) return false;
    det = 1.0f / det;
    for (int i = 0; i < 16; ++i) out.m[i] = inv[i] * det;
    return true;
}

} // namespace zamin
