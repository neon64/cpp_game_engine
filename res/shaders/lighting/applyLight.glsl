vec3 applyLight(Light light, float scaleIntensity, vec3 surfaceColor, vec3 normal, vec3 surfacePosition, vec3 surfaceToCamera, float scaleAmbientAndDiffuse) {
    // ambient
    vec3 ambient = light.ambientCoefficient * surfaceColor * light.color;

    vec3 surfaceToLight;
    float intensity = 1.0;
    if(light.range == -1.0) { // directional light
        surfaceToLight = normalize(light.position);
    } else { // point light
        surfaceToLight = normalize(light.position - surfacePosition);
        float distanceToLight = length(light.position - surfacePosition);

//        if(distanceToLight > light.range) {
//            return ambient;
//        }

        intensity = 1.0 / (light.attenuation.x + // constant
                      light.attenuation.y * distanceToLight + // linear
                      light.attenuation.z * distanceToLight * distanceToLight + // exponent
                      0.0001);

        // cone restrictions
        if(light.outerConeAngle > 0.0) {
            float lightToSurfaceAngle = degrees(acos(dot(-surfaceToLight, normalize(light.direction))));
            float innerMinusOuter = light.innerConeAngle - light.outerConeAngle;

            intensity *= clamp((lightToSurfaceAngle - light.outerConeAngle) /
            	       innerMinusOuter, 0.0, 1.0);

            /* if(lightToSurfaceAngle > light.coneAngle) {
                return ambient;
            } else {
                //intensity /= lightToSurfaceAngle / light.coneAngle;
            } */
        }
    }

    // diffuse
    float diffuseCoefficient = max(0.0, dot(normal, surfaceToLight));
    vec3 diffuse = diffuseCoefficient * surfaceColor * light.color;

    // blinn-phong specular component
    vec3 halfwayDir = normalize(surfaceToLight + surfaceToCamera);
    float specularCoefficient = pow(max(dot(normal, halfwayDir), 0.0), materialShininess);

    // phong specular component
//    vec3 reflectDir = reflect(-surfaceToLight, normal);
//    float specularCoefficient = pow(max(0.0, dot(surfaceToCamera, reflect(-surfaceToLight, normal))), materialShininess);

    vec3 specular = specularCoefficient * materialSpecularColor * light.color;

    return max(ambient * scaleAmbientAndDiffuse, scaleIntensity * intensity * (diffuse * scaleAmbientAndDiffuse + specular));

    return (ambient * scaleAmbientAndDiffuse + scaleIntensity * intensity * (diffuse * scaleAmbientAndDiffuse + specular)) * light.color;
}