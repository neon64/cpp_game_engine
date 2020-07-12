layout(std140, binding = 1) uniform LightingBlock {
    #if NUM_SHADOW_CASTING_LIGHTS > 0
        mat4 lightMatrices[NUM_SHADOW_CASTING_LIGHTS];
        float shadowMinVariance[NUM_SHADOW_CASTING_LIGHTS];
        float shadowLightBleedReduction[NUM_SHADOW_CASTING_LIGHTS];
        Light allShadowCastingLights[NUM_SHADOW_CASTING_LIGHTS];
    #endif
    #if NUM_LIGHTS > 0
        Light allLights[NUM_LIGHTS];
    #endif

};