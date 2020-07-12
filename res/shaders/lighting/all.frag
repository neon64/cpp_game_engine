#version 420

layout(std140, binding = 3) uniform Material {
    // needed for lighting calculations
    vec3 materialSpecularColor;
    float materialShininess;
    #if USE_PARALLAX_DISPLACEMENT_MAP
        float dispMapScale;
        float dispMapBias;
    #endif
    #if USE_FLAT_COLOR
        vec4 materialColor;
    #endif
};

#include "light.glsl"
#include "applyLight.glsl"
#include "fog.glsl"

// shadows

#if NUM_SHADOW_CASTING_LIGHTS > 0
    in vec4 shadowMapCoordinates[NUM_SHADOW_CASTING_LIGHTS];
    layout(binding = 8) uniform sampler2D shadowMaps[NUM_SHADOW_CASTING_LIGHTS];
#endif

#include "lightingBlock.glsl"

#if NUM_SHADOW_CASTING_LIGHTS > 0
    #include "../sampleShadowMap.glsl"
#endif

// for fog
in vec3 eyeSpacePosition;

// for shadows
in vec3 surfacePosition;

layout(location = 0) out vec4 finalColor;

#include "matrixBlock.glsl"

#if HAS_TEXTURE_COORDINATE
    in vec2 textureCoordinate;
#endif


#if USE_NORMAL_MAP
    // normal map
    layout(binding = 1) uniform sampler2D normalMap;
    in mat3 tbnMatrix;
#else
    flat in mat3 fragNormalMatrix;
    in vec3 passNormal;
#endif

#if USE_PARALLAX_DISPLACEMENT_MAP
    // parallax displacement map
    uniform sampler2D dispMap;

    vec2 calcParallaxTexCoords(sampler2D dispMap, mat3 tbnMatrix, vec3 directionToEye, vec2 texCoords, float scale, float bias) {
        return texCoords + (directionToEye * tbnMatrix).xy * (texture(dispMap, texCoords).r * scale + bias);
    }
#endif

#if USE_HEIGHTMAP
#include "sampleHeightmap.glsl"
#elif USE_COLOR_TEXTURE
    layout(binding = 0) uniform sampler2D materialTexture;
#endif

void main() {
    vec3 surfaceToCamera = normalize(cameraPosition - surfacePosition);

    #if USE_PARALLAX_DISPLACEMENT_MAP
        vec2 surfaceTextureCoordinate = calcParallaxTexCoords(dispMap, tbnMatrix, surfaceToCamera, textureCoordinate, dispMapScale, dispMapBias);
    #elif HAS_TEXTURE_COORDINATE
        vec2 surfaceTextureCoordinate = textureCoordinate;
    #endif

    #if USE_HEIGHTMAP
        vec4 surfaceColor = sampleHeightmap(surfaceTextureCoordinate);
    #elif USE_COLOR_TEXTURE
        vec4 surfaceColor = texture(materialTexture, surfaceTextureCoordinate);
    #else
        vec4 surfaceColor = materialColor;
    #endif

    #if USE_NORMAL_MAP
        vec3 surfaceNormal = normalize(tbnMatrix * (255.0/128.0 * texture(normalMap, surfaceTextureCoordinate).xyz - 1));
    #else
        vec3 surfaceNormal = normalize(fragNormalMatrix * passNormal);
    #endif

    vec3 linearColor = vec3(0);

    #if NUM_LIGHTS > 0
        for(int i = 0; i < NUM_LIGHTS; ++i) {
            linearColor += applyLight(allLights[i], 1.0, surfaceColor.rgb, surfaceNormal, surfacePosition, surfaceToCamera, 1.0);
        }
    #endif

    #if NUM_SHADOW_CASTING_LIGHTS > 0
        for(int i = 0; i < NUM_SHADOW_CASTING_LIGHTS; ++i) {
            float shadowAmount = calculateShadow(shadowMaps[i], shadowMapCoordinates[i], shadowMinVariance[i], shadowLightBleedReduction[i]);
            linearColor += applyLight(allShadowCastingLights[i], shadowAmount, surfaceColor.rgb, surfaceNormal, surfacePosition, surfaceToCamera, 1.0);
        }
    #endif

    #if APPLY_FOG
        linearColor = applyFog(
        linearColor,
        length(eyeSpacePosition.xyz),
        surfaceToCamera, // coordinate to sample the skybox texture
        normalize(allShadowCastingLights[0].position.xyz)
        );
    #endif

    finalColor = vec4(linearColor, surfaceColor.a);
}