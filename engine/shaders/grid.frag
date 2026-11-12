#version 450

layout(push_constant) uniform Push {
    mat4 viewProj;
    vec4 n0; // n0.w: 1 = chiqish sRGB
    vec4 n1;
    vec4 n2; // n2.w: 1 = 2D rejim
    vec4 color;
} pc;

layout(location = 0) in vec2 vPlane;
layout(location = 0) out vec4 outColor;

float gridLine(vec2 coord) {
    vec2 g = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
    return 1.0 - min(min(g.x, g.y), 1.0);
}

vec3 toOut(vec3 c) { return pc.n0.w > 0.5 ? pow(c, vec3(2.2)) : c; }

void main() {
    bool is2D = pc.n2.w > 0.5;
    vec2 uv = vPlane;
    float minor = gridLine(uv);          // 1 m
    float major = gridLine(uv / 10.0);   // 10 m

    vec3 color = vec3(0.42, 0.45, 0.52);
    float alpha = max(minor * 0.22, major * 0.45);

    // O'qlar. 3D: X — qizil, Z — ko'k. 2D: X — qizil, Y — yashil.
    vec2 d = fwidth(uv);
    float axisA = 1.0 - min(abs(uv.y) / (d.y * 1.5), 1.0); // X o'qi
    float axisB = 1.0 - min(abs(uv.x) / (d.x * 1.5), 1.0); // Z (3D) yoki Y (2D) o'qi
    vec3 colB = is2D ? vec3(0.45, 0.80, 0.35) : vec3(0.30, 0.52, 0.95);
    if (axisA > 0.0) { color = mix(color, vec3(0.90, 0.30, 0.32), axisA); alpha = max(alpha, axisA * 0.85); }
    if (axisB > 0.0) { color = mix(color, colB, axisB); alpha = max(alpha, axisB * 0.85); }

    // Uzoqda so'nish (2D da ortografik — kengroq)
    float fade = 1.0 - smoothstep(is2D ? 40.0 : 10.0, is2D ? 110.0 : 45.0, length(uv));
    outColor = vec4(toOut(color), alpha * fade);
}
