#version 420

#include "light.glsl"

layout (location = 0) in vec3 vertexPosition;

#if HAS_TEXTURE_COORDINATE
    layout (location = 1) in vec2 vertexTexCoord;
    layout (location = 2) in vec3 vertexNormal;
    out vec2 textureCoordinate;
#else
    layout (location = 2) in vec3 vertexNormal;
#endif

#if USE_NORMAL_MAP
    layout (location = 3) in vec3 vertexTangent;
    out mat3 tbnMatrix;
#else
    flat out mat3 fragNormalMatrix;
    out vec3 passNormal;
#endif

layout(location = 5) in mat4 modelMatrix;
layout(location = 9) in mat3 normalMatrix;

// for fog
out vec3 eyeSpacePosition;

// for shadows
out vec3 surfacePosition;

#if CLIP_PLANE
    // for water
    out float gl_ClipDistance[1];
#endif

#include "matrixBlock.glsl"

#if USE_HEIGHTMAP
    out float vertexHeight;
#endif

#include "lightingBlock.glsl"

#if NUM_SHADOW_CASTING_LIGHTS > 0
    out vec4 shadowMapCoordinates[NUM_SHADOW_CASTING_LIGHTS];
#endif


void main() {
    #if HAS_TEXTURE_COORDINATE
        textureCoordinate = vertexTexCoord;
    #endif
    surfacePosition = vec3(modelMatrix * vec4(vertexPosition, 1.0));
    eyeSpacePosition = vec3(viewMatrix * vec4(surfacePosition, 1.0));

    #if USE_HEIGHTMAP
        vertexHeight = vertexPosition.y;
    #endif

    // Apply all matrix transformations to vertices
    gl_Position = projectionMatrix * vec4(eyeSpacePosition,1.0);

    #if USE_NORMAL_MAP
        vec3 normal = normalize(normalMatrix * vertexNormal);
        vec3 tangent = normalize(normalMatrix * vertexTangent);

        // re-orthogonalize tangent with respect to normal using gram-schmitt process
        // https://learnopengl.com/Advanced-Lighting/Normal-Mapping
        tangent = normalize(tangent - dot(tangent, normal) * normal);
        vec3 biTangent = cross(tangent, normal);

        tbnMatrix = mat3(tangent, biTangent, normal);
    #else
        fragNormalMatrix = normalMatrix;
        passNormal = vertexNormal;
    #endif

    // Calculate shadow map coordinates
    #if NUM_SHADOW_CASTING_LIGHTS > 0
        for(int i = 0; i < NUM_SHADOW_CASTING_LIGHTS; ++i) {
            shadowMapCoordinates[i] = lightMatrices[i] * vec4(surfacePosition, 1.0);
        }
    #endif

    #if CLIP_PLANE
        gl_ClipDistance[0] = dot(vec4(surfacePosition, 1.0), clipPlane);
    #endif
}