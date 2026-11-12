#version 450

layout(push_constant) uniform Push {
    mat4 viewProj;
    vec4 params;
} pc;

layout(location = 0) in vec3 vWorld;
layout(location = 0) out vec4 outColor;

float gridLine(vec2 coord) {
    vec2 g = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
    return 1.0 - min(min(g.x, g.y), 1.0);
}

vec3 toOut(vec3 c) { return pc.params.x > 0.5 ? pow(c, vec3(2.2)) : c; }

void main() {
    vec2 xz = vWorld.xz;
    float minor = gridLine(xz);          // 1 m
    float major = gridLine(xz / 10.0);   // 10 m

    vec3 color = vec3(0.42, 0.45, 0.52);
    float alpha = max(minor * 0.22, major * 0.45);

    // O'qlar: X — qizil (z = 0), Z — ko'k (x = 0)
    vec2 d = fwidth(xz);
    float axisX = 1.0 - min(abs(xz.y) / (d.y * 1.5), 1.0);
    float axisZ = 1.0 - min(abs(xz.x) / (d.x * 1.5), 1.0);
    if (axisX > 0.0) { color = mix(color, vec3(0.90, 0.30, 0.32), axisX); alpha = max(alpha, axisX * 0.85); }
    if (axisZ > 0.0) { color = mix(color, vec3(0.30, 0.52, 0.95), axisZ); alpha = max(alpha, axisZ * 0.85); }

    // Uzoqda so'nish
    float fade = 1.0 - smoothstep(8.0, 35.0, length(xz));
    outColor = vec4(toOut(color), alpha * fade);
}
