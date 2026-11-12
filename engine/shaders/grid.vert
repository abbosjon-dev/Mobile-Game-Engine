#version 450
// Yer to'ri: 3D da y = 0 (XZ), 2D da z = 0 (XY) tekisligidagi katta kvadrat

layout(push_constant) uniform Push {
    mat4 viewProj;
    vec4 n0; // n0.w: 1 = chiqish sRGB
    vec4 n1;
    vec4 n2; // n2.w: 1 = 2D rejim (to'r XY tekisligida)
    vec4 color;
} pc;

layout(location = 0) out vec2 vPlane;

const float SIZE = 120.0;
const vec2 C[6] = vec2[](vec2(-1,-1), vec2(1,-1), vec2(1,1), vec2(-1,-1), vec2(1,1), vec2(-1,1));

void main() {
    vec2 q = C[gl_VertexIndex] * SIZE;
    vPlane = q;
    // 3D: kichik ko'tarilish — y = 0 dagi tekisliklar bilan z-fighting bo'lmasin
    vec3 p = pc.n2.w > 0.5 ? vec3(q.x, q.y, -0.01) : vec3(q.x, 0.002, q.y);
    gl_Position = pc.viewProj * vec4(p, 1.0);
}
