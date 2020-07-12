layout(std140, binding = 0) uniform MatrixBlock {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec3 cameraPosition;
    #if CLIP_PLANE
        vec4 clipPlane;
    #endif
};