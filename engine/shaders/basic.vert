#version 450
// Bosqich 0: vertex buffer'siz piramida (gl_VertexIndex orqali)

layout(push_constant) uniform Push {
    mat4 mvp;
    vec4 params; // x: 1 = sRGB chiqish
} pc;

layout(location = 0) out vec3 vColor;

const vec3 APEX = vec3(0.0, 0.8, 0.0);
const vec3 B0 = vec3(-0.7, -0.5, -0.7);
const vec3 B1 = vec3( 0.7, -0.5, -0.7);
const vec3 B2 = vec3( 0.7, -0.5,  0.7);
const vec3 B3 = vec3(-0.7, -0.5,  0.7);

const vec3 POS[18] = vec3[](
    APEX, B3, B2,   // old
    APEX, B2, B1,   // o'ng
    APEX, B1, B0,   // orqa
    APEX, B0, B3,   // chap
    B0, B1, B2,     // asos
    B0, B2, B3
);

// Zamin palitrasi: yashil, sariq, ko'k, to'q sariq, yer rangi
const vec3 FACE[6] = vec3[](
    vec3(0.24, 0.73, 0.54),
    vec3(0.95, 0.70, 0.24),
    vec3(0.30, 0.55, 0.95),
    vec3(0.92, 0.42, 0.30),
    vec3(0.16, 0.49, 0.37),
    vec3(0.16, 0.49, 0.37)
);

void main() {
    int i = gl_VertexIndex;
    gl_Position = pc.mvp * vec4(POS[i], 1.0);

    // Yassi soyalash: uchburchak normali (obyekt fazosida) va qat'iy yorug'lik
    int t = (i / 3) * 3;
    vec3 n = normalize(cross(POS[t + 1] - POS[t], POS[t + 2] - POS[t]));
    vec3 lightDir = normalize(vec3(0.5, 0.9, 0.6));
    float diffuse = abs(dot(n, lightDir));
    float shade = 0.62 + 0.45 * diffuse;
    vec3 c = min(FACE[i / 3] * shade, vec3(1.0));
    // Palitra sRGB da; sRGB framebuffer uchun linearga o'tkazamiz
    vColor = pc.params.x > 0.5 ? pow(c, vec3(2.2)) : c;
}
