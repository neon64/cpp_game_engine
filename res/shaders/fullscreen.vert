#version 420

layout(location = 0) in vec3 vertexPosition;

out vec2 texCoord;

void main() {
    texCoord = (vertexPosition.xy + 1.0) * 0.5;
    gl_Position = vec4(vertexPosition, 1.0);
}