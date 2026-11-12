#version 450
// Sahna obyektlari: pozitsiya + normal, yarim-sfera (hemisphere) + yo'naltirilgan yorug'lik

layout(push_constant) uniform Push {
    mat4 mvp;
    vec4 n0; // normal matritsa ustunlari; n0.w: 1 = chiqish sRGB
    vec4 n1; //                           n1.w: 1 = yoritilmagan (outline)
    vec4 n2;
    vec4 color;
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 0) out vec3 vColor;

void main() {
    gl_Position = pc.mvp * vec4(inPos, 1.0);

    vec3 c;
    if (pc.n1.w > 0.5) {
        c = pc.color.rgb;
    } else {
        vec3 n = normalize(mat3(pc.n0.xyz, pc.n1.xyz, pc.n2.xyz) * inNormal);
        vec3 lightDir = normalize(vec3(0.45, 0.85, 0.35));
        float diffuse = max(dot(n, lightDir), 0.0);
        vec3 sky = vec3(0.70, 0.74, 0.82);
        vec3 ground = vec3(0.46, 0.43, 0.40);
        vec3 ambient = mix(ground, sky, n.y * 0.5 + 0.5);
        c = pc.color.rgb * (ambient * 0.72 + diffuse * 0.5);
    }
    c = min(c, vec3(1.0));
    // Ranglar sRGB da; sRGB framebuffer uchun linearga o'tkazamiz
    vColor = pc.n0.w > 0.5 ? pow(c, vec3(2.2)) : c;
}
