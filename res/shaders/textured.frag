#version 420

in vec3 normal;
in vec2 texCoord;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D diffuseTexture;

void main(void) {
    color = vec4(texture(diffuseTexture, texCoord).rgb, 1.0f);
}
