#version 420

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in mat4 modelMatrix;

struct Light {
    float x;
    float y;
    float z;
};

layout(std140, binding = 0) uniform MatrixBlock {
    mat4 viewProjectionMatrix;
    // Light[5] foo;
};

out vec2 texCoord;
out vec3 normal;

void main(void) {
    //vec3 offset = vec3(0.5f * (gl_InstanceID % 10), 0, 0.5f * (gl_InstanceID / 10));
    gl_Position = viewProjectionMatrix * modelMatrix * vec4(vertexPosition, 1.0);
    texCoord = vertexTexCoord;
    normal = vertexNormal;
}
