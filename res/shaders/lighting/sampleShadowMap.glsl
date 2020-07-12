/**
 * Samples from a basic shadow map.
 */
float sampleShadowMap(sampler2D shadowMap, vec2 coords, float compare) {
    return step(compare, texture(shadowMap, coords).r);
}

/**
 * Samples a shadow map using basic linear filtering,
 * as if was sampled from a Texture with a filter set to LINEAR,
 * but for shadow maps.
 */
float sampleShadowMapLinear(sampler2D shadowMap, vec2 coords, float compare, vec2 texelSize) {
    vec2 pixelPosition = coords / texelSize + vec2(0.5); // 'un-normalizes' the coordinates and centers them
    vec2 fractionalPart = fract(pixelPosition); // eg 512.5 becomes 0.5
    vec2 startTexel = (pixelPosition - fractionalPart) * texelSize; // eg 512.5 becomes 512 => 512 / 1024 => 0.5

    float blTexel = sampleShadowMap(shadowMap, startTexel, compare);
    float brTexel = sampleShadowMap(shadowMap, startTexel + vec2(texelSize.x, 0.0), compare);
    float tlTexel = sampleShadowMap(shadowMap, startTexel + vec2(0.0, texelSize.y), compare);
    float trTexel = sampleShadowMap(shadowMap, startTexel + texelSize, compare);

    float mixedLeft = mix(blTexel, tlTexel, fractionalPart.y);
    float mixedRight = mix(brTexel, trTexel, fractionalPart.y);

    return mix(mixedLeft, mixedRight, fractionalPart.x);
}

/**
 * Samples a shadow map using percentage closer filtering.
 */
float sampleShadowMapPCF(sampler2D shadowMap, vec2 coords, float compare, vec2 texelSize) {
    const float NUM_SAMPLES = 3.0f;
    const float SAMPLES_START = (NUM_SAMPLES - 1.0f) / 2.0f;
    const float NUM_SAMPLES_SQUARED = NUM_SAMPLES * NUM_SAMPLES;

    float result = 0.0f;

    // averages out the different samples
    for(float height = -SAMPLES_START; height <= SAMPLES_START; ++height) {
        for(float width = -SAMPLES_START; width <= SAMPLES_START; ++width) {
           vec2 coordsOffset = vec2(width, height) * texelSize;

           result += sampleShadowMapLinear(shadowMap, coords + coordsOffset, compare, texelSize);
        }
    }

    return result / NUM_SAMPLES_SQUARED;
}

float linstep(float low, float high, float v) {
    return clamp((v - low) / (high-low), 0.0, 1.0);
}

/**
 * Samples a value from a variance shadow map.
 *
 */
float sampleVarianceShadowMap(sampler2D shadowMap, vec2 coords, float compare, float varianceMin, float lightBleedReduction) {
    vec2 sampled = texture(shadowMap, coords).xy;

    // is the average in shadow or not?
    float p = step(compare, sampled.x);

    // the variance of the depths
    float variance = max(sampled.y - sampled.x * sampled.x, varianceMin);

    // distance from the mean
    float d = compare - sampled.x;

    // maximum percentage that are >= compare
    float pMax = linstep(lightBleedReduction, 1.0, variance / (variance + d * d));

    return min(max(p, pMax), 1.0);
}

bool inRange(float n) {
    return n > 0.0 && n < 1.0;
}

/**
 * Calculates the amount of shadow.
 */
float calculateShadow(sampler2D shadowMap, vec4 initialCoords, float varianceMin, float lightBleedReduction) {
    vec3 coords = (initialCoords.xyz / initialCoords.w);

    if(!inRange(coords.x) || !inRange(coords.y) || !inRange(coords.z)) {
        return 1.0;
    }

    return sampleVarianceShadowMap(shadowMap, coords.xy, coords.z, varianceMin, lightBleedReduction)*0.99 + 0.01;
}