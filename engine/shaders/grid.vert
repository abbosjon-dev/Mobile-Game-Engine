#version 450
// Cheksiz ko'rinadigan yer to'ri: y = -0.5 tekisligidagi katta kvadrat

layout(push_constant) uniform Push {
    mat4 viewProj;
    vec4 params;
} pc;

layout(location = 0) out vec3 vWorld;

const float SIZE = 80.0;
const float Y = -0.5;
const vec2 C[6] = vec2[](vec2(-1,-1), vec2(1,-1), vec2(1,1), vec2(-1,-1), vec2(1,1), vec2(-1,1));

void main() {
    vec3 p = vec3(C[gl_VertexIndex].x * SIZE, Y, C[gl_VertexIndex].y * SIZE);
    vWorld = p;
    gl_Position = pc.viewProj * vec4(p, 1.0);
}
