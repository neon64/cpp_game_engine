uniform sampler2D materialTextures[5];
uniform float textureCoordinateScale;

in float vertexHeight;

vec4 sampleHeightmap(in vec2 textureCoordinate) {
    float scale = vertexHeight;

    const float range1 = 0.15f;
    const float range2 = 0.3f;
    const float range3 = 0.65f;
    const float range4 = 0.85f;

    vec2 scaledCoord = textureCoordinate * textureCoordinateScale;

    vec4 surfaceColor = vec4(0.0);

    if(scale >= 0.0 && scale <= range1) {
        surfaceColor = texture(materialTextures[0], scaledCoord);
    } else if(scale <= range2) {
        scale -= range1;
        scale /= (range2-range1);

        float scale2 = scale;
        scale = 1.0-scale;

        surfaceColor += texture(materialTextures[0], scaledCoord) * scale;
        surfaceColor += texture(materialTextures[1], scaledCoord) * scale2;
    } else if(scale <= range3) {
        surfaceColor = texture(materialTextures[1], scaledCoord);
    } else if(scale <= range4) {
        scale -= range3;
        scale /= (range4-range3);

        float scale2 = scale;
        scale = 1.0-scale;

        surfaceColor += texture(materialTextures[1], scaledCoord)*scale;
        surfaceColor += texture(materialTextures[2], scaledCoord)*scale2;
    } else {
        surfaceColor = texture(materialTextures[2], scaledCoord);
    }

    return surfaceColor;
}

