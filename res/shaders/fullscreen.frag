#version 420

layout(binding = 0) uniform sampler2D tex;
layout(binding = 1) uniform sampler2D colorTex;

const float zNear = 0.01;
const float zFar = 10.0;

in vec2 texCoord;
out vec4 color;

void main() {
    float z_b = texture(tex, texCoord).r;
    vec3 c = texture(colorTex, texCoord).rgb;

    float z_n = 2.0 * z_b - 1.0;
    float z_e = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));

    color = vec4(1.0 - z_e, 1.0 - z_e, 1.0 - z_e, 1.0);
}