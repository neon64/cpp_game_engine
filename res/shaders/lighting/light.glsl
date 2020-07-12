struct Light {
    // for a directional light, the position *is* the direction
    vec3 position;
    float range;
    vec3 color;
    float ambientCoefficient;
    vec3 direction;
    float innerConeAngle;
    vec3 attenuation;
    float outerConeAngle;
};