uniform struct FogInfo {
   vec3 color; // Fog color
   vec3 sunColor; // Color when facing the sun
   float start; // This is only for linear fog
   float end; // This is only for linear fog
   float density; // For exp and exp2 equation
   float gradient;
   int equation; // 0 = linear, 1 = exp, 2 = exp2
} fogParams;

uniform samplerCube fogSkybox;

/**
 * Based upon this.
 * https://www.iquilezles.org/www/articles/fog/fog.htm
 */

// original color of the pixel,
// and the fog coordinate
vec3 applyFog(in vec3 rgb, in float distance, in vec3 surfaceToCamera, in vec3 sunDir) {
   float result = 0.0;
   if(fogParams.equation == 0) // linear
      result = (fogParams.end - distance)/(fogParams.end - fogParams.start);
   else if(fogParams.equation == 1) { // exp
      result = exp(-fogParams.density * distance);
   } else if(fogParams.equation == 2) { // exp2
      result = exp(-pow(fogParams.density * distance, fogParams.gradient));
   }

   float sunAmount = max(0.0, -dot(surfaceToCamera, sunDir));

   //float mipmapLevel = textureQueryLod(fogSkybox, -surfaceToCamera).x;
   //vec3 fogBaseColor = textureLod(fogSkybox, -surfaceToCamera, mipmapLevel+4).xyz;

//   vec3 fogColor = mix(
//      fogParams.color, // bluish
//      fogParams.sunColor, // yellowish,
//      clamp(pow(sunAmount, 2.0), 0.0, 1.0)
//   );

   vec3 fogColor = fogParams.color;

   float fogAmount = 1.0 - clamp(result, 0.0, 1.0);

   return rgb;

   return mix(rgb, fogColor, fogAmount);
}