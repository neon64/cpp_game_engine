#include "lighting_test.h"
#include <memory>
namespace pipelines { namespace lighting_test {
const char* VERTEX_SHADER = R""(
#version 420
#extension GL_ARB_shading_language_include : enable

#line 1 "/home/chris/code/game_engine/res/shaders/lighting/light.glsl"
struct Light {

    vec3 position;
    float range;
    vec3 color;
    float ambientCoefficient;
    vec3 direction;
    float innerConeAngle;
    vec3 attenuation;
    float outerConeAngle;
};
#line 4 "source"

layout(location = 0)in vec3 vertexPosition;


    layout(location = 1)in vec2 vertexTexCoord;
    layout(location = 2)in vec3 vertexNormal;
    out vec2 textureCoordinate;





    layout(location = 3)in vec3 vertexTangent;
    out mat3 tbnMatrix;





layout(location = 5)in mat4 modelMatrix;
layout(location = 9)in mat3 normalMatrix;


out vec3 eyeSpacePosition;


out vec3 surfacePosition;






#line 1 "/home/chris/code/game_engine/res/shaders/lighting/matrixBlock.glsl"
layout(std140, binding = 0)uniform MatrixBlock {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec3 cameraPosition;



};
#line 38 "source"





#line 1 "/home/chris/code/game_engine/res/shaders/lighting/lightingBlock.glsl"
layout(std140, binding = 1)uniform LightingBlock {







        Light allLights[1];


};
#line 44 "source"






void main(){

        textureCoordinate = vertexTexCoord;

    surfacePosition = vec3(modelMatrix * vec4(vertexPosition, 1.0));
    eyeSpacePosition = vec3(viewMatrix * vec4(surfacePosition, 1.0));






    gl_Position = projectionMatrix * vec4(eyeSpacePosition, 1.0);


        vec3 normal = normalize(normalMatrix * vertexNormal);
        vec3 tangent = normalize(normalMatrix * vertexTangent);



        tangent = normalize(tangent - dot(tangent, normal)* normal);
        vec3 biTangent = cross(tangent, normal);

        tbnMatrix = mat3(tangent, biTangent, normal);















}
)"";
string VertexShader::getKey() const { return key; }
shared_ptr<Shader> VertexShader::build(OpenGLContext& context) {
    return make_shared<Shader>(std::move(context.buildShader(ShaderType::VERTEX, key, VERTEX_SHADER)));
}
const char* FRAGMENT_SHADER = R""(
#version 420
#extension GL_ARB_shading_language_include : enable

layout(std140, binding = 3)uniform Material {

    vec3 materialSpecularColor;
    float materialShininess;







};

#line 1 "/home/chris/code/game_engine/res/shaders/lighting/light.glsl"
struct Light {

    vec3 position;
    float range;
    vec3 color;
    float ambientCoefficient;
    vec3 direction;
    float innerConeAngle;
    vec3 attenuation;
    float outerConeAngle;
};
#line 17 "source"
#line 1 "/home/chris/code/game_engine/res/shaders/lighting/applyLight.glsl"
vec3 applyLight(Light light, float scaleIntensity, vec3 surfaceColor, vec3 normal, vec3 surfacePosition, vec3 surfaceToCamera, float scaleAmbientAndDiffuse){

    vec3 ambient = light . ambientCoefficient * surfaceColor * light . color;

    vec3 surfaceToLight;
    float intensity = 1.0;
    if(light . range == - 1.0){
        surfaceToLight = normalize(light . position);
    } else {
        surfaceToLight = normalize(light . position - surfacePosition);
        float distanceToLight = length(light . position - surfacePosition);





        intensity = 1.0 /(light . attenuation . x +
                      light . attenuation . y * distanceToLight +
                      light . attenuation . z * distanceToLight * distanceToLight +
                      0.0001);


        if(light . outerConeAngle > 0.0){
            float lightToSurfaceAngle = degrees(acos(dot(- surfaceToLight, normalize(light . direction))));
            float innerMinusOuter = light . innerConeAngle - light . outerConeAngle;

            intensity *= clamp((lightToSurfaceAngle - light . outerConeAngle)/
                    innerMinusOuter, 0.0, 1.0);






        }
    }


    float diffuseCoefficient = max(0.0, dot(normal, surfaceToLight));
    vec3 diffuse = diffuseCoefficient * surfaceColor * light . color;


    vec3 halfwayDir = normalize(surfaceToLight + surfaceToCamera);
    float specularCoefficient = pow(max(dot(normal, halfwayDir), 0.0), materialShininess);





    vec3 specular = specularCoefficient * materialSpecularColor * light . color;

    return max(ambient * scaleAmbientAndDiffuse, scaleIntensity * intensity *(diffuse * scaleAmbientAndDiffuse + specular));

    return(ambient * scaleAmbientAndDiffuse + scaleIntensity * intensity *(diffuse * scaleAmbientAndDiffuse + specular))* light . color;
}
#line 18 "source"
#line 1 "/home/chris/code/game_engine/res/shaders/lighting/fog.glsl"
 uniform struct FogInfo {
   vec3 color;
   vec3 sunColor;
   float start;
   float end;
   float density;
   float gradient;
   int equation;
} fogParams;

uniform samplerCube fogSkybox;








vec3 applyFog(in vec3 rgb, in float distance, in vec3 surfaceToCamera, in vec3 sunDir){
   float result = 0.0;
   if(fogParams . equation == 0)
      result =(fogParams . end - distance)/(fogParams . end - fogParams . start);
   else if(fogParams . equation == 1){
      result = exp(- fogParams . density * distance);
   } else if(fogParams . equation == 2){
      result = exp(- pow(fogParams . density * distance, fogParams . gradient));
   }

   float sunAmount = max(0.0, - dot(surfaceToCamera, sunDir));










   vec3 fogColor = fogParams . color;

   float fogAmount = 1.0 - clamp(result, 0.0, 1.0);

   return rgb;

   return mix(rgb, fogColor, fogAmount);
}
#line 19 "source"








#line 1 "/home/chris/code/game_engine/res/shaders/lighting/lightingBlock.glsl"
 layout(std140, binding = 1)uniform LightingBlock {







        Light allLights[1];


};
#line 28 "source"






in vec3 eyeSpacePosition;


in vec3 surfacePosition;

layout(location = 0)out vec4 finalColor;

#line 1 "/home/chris/code/game_engine/res/shaders/lighting/matrixBlock.glsl"
layout(std140, binding = 0)uniform MatrixBlock {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec3 cameraPosition;



};
#line 42 "source"


    in vec2 textureCoordinate;





    layout(binding = 1)uniform sampler2D normalMap;
    in mat3 tbnMatrix;

















    layout(binding = 0)uniform sampler2D materialTexture;


void main(){
    vec3 surfaceToCamera = normalize(cameraPosition - surfacePosition);




        vec2 surfaceTextureCoordinate = textureCoordinate;





        vec4 surfaceColor = texture(materialTexture, surfaceTextureCoordinate);





        vec3 surfaceNormal = normalize(tbnMatrix *(255.0 / 128.0 * texture(normalMap, surfaceTextureCoordinate). xyz - 1));




    vec3 linearColor = vec3(0);


        for(int i = 0;i < 1;++ i){
            linearColor += applyLight(allLights[i], 1.0, surfaceColor . rgb, surfaceNormal, surfacePosition, surfaceToCamera, 1.0);
        }


















    finalColor = vec4(linearColor, surfaceColor . a);
}
)"";
string FragmentShader::getKey() const { return key; }
shared_ptr<Shader> FragmentShader::build(OpenGLContext& context) {
    return make_shared<Shader>(std::move(context.buildShader(ShaderType::FRAGMENT, key, FRAGMENT_SHADER)));
}
Shaders::Shaders(ShaderCache* cache) : cache(*cache) {}
Shaders::Shaders(ShaderCache& cache) : cache(cache) {}
ShaderStages Shaders::getStages() const {
    return {
    .vertex = cache.get(VertexShader {}),
    .fragment = cache.get(FragmentShader {}),
    };
};
void VertexBindingPipelineState::bindAll(const VertexBindings& bindings, BoundVertexArrayGuard& guard, OpenGLContext& context) {
    guard.bindVertexBuffer(0, bindings.perVertex.buffer, bindings.perVertex.byteOffset, sizeof(VertexInput));
    guard.bindVertexBuffer(1, bindings.perInstance.buffer, bindings.perInstance.byteOffset, sizeof(InstanceInput));
}
VertexBindingPipelineState VertexBindingCreateInfo::init(VertexArray& array, OpenGLContext& context) {
    context.withBoundVertexArray(array, [](auto guard) {
        guard.enableAttribute(1);
        guard.enableAttribute(0);
        guard.enableAttribute(2);
        guard.enableAttribute(3);
        guard.enableAttribute(5);
        guard.enableAttribute(6);
        guard.enableAttribute(7);
        guard.enableAttribute(8);
        guard.enableAttribute(9);
        guard.enableAttribute(10);
        guard.enableAttribute(11);
        guard.setAttributeFormat(1, DataFormat::R32G32_SFLOAT, offsetof(VertexInput, texCoord));
        guard.setAttributeFormat(0, DataFormat::R32G32B32_SFLOAT, offsetof(VertexInput, position));
        guard.setAttributeFormat(2, DataFormat::R32G32B32_SFLOAT, offsetof(VertexInput, normal));
        guard.setAttributeFormat(3, DataFormat::R32G32B32_SFLOAT, offsetof(VertexInput, tangent));
        guard.setAttributeFormat(5, DataFormat::R32G32B32A32_SFLOAT, offsetof(InstanceInput, modelMatrix.column0));
        guard.setAttributeFormat(6, DataFormat::R32G32B32A32_SFLOAT, offsetof(InstanceInput, modelMatrix.column1));
        guard.setAttributeFormat(7, DataFormat::R32G32B32A32_SFLOAT, offsetof(InstanceInput, modelMatrix.column2));
        guard.setAttributeFormat(8, DataFormat::R32G32B32A32_SFLOAT, offsetof(InstanceInput, modelMatrix.column3));
        guard.setAttributeFormat(9, DataFormat::R32G32B32_SFLOAT, offsetof(InstanceInput, normalMatrix.column0));
        guard.setAttributeFormat(10, DataFormat::R32G32B32_SFLOAT, offsetof(InstanceInput, normalMatrix.column1));
        guard.setAttributeFormat(11, DataFormat::R32G32B32_SFLOAT, offsetof(InstanceInput, normalMatrix.column2));
        guard.setAttributeBinding(1, 0);
        guard.setAttributeBinding(0, 0);
        guard.setAttributeBinding(2, 0);
        guard.setAttributeBinding(3, 0);
        guard.setAttributeBinding(5, 1);
        guard.setAttributeBinding(6, 1);
        guard.setAttributeBinding(7, 1);
        guard.setAttributeBinding(8, 1);
        guard.setAttributeBinding(9, 1);
        guard.setAttributeBinding(10, 1);
        guard.setAttributeBinding(11, 1);
        guard.setBindingDivisor(1, 1);
    });
    return VertexBindingPipelineState {};
}
void ResourceBindingPipelineState::bindAll(const ResourceBindings& bindings, OpenGLContext& context) {
    context.bindUniformBuffer(bindings.matrixBlock.buffer, 0, bindings.matrixBlock.byteOffset, sizeof(MatrixBlock));
    context.bindUniformBuffer(bindings.material.buffer, 3, bindings.material.byteOffset, sizeof(Material));
    context.bindUniformBuffer(bindings.lightingBlock.buffer, 1, bindings.lightingBlock.byteOffset, sizeof(LightingBlock));
    context.bindTextureAndSampler(0, bindings.materialTexture);
    context.bindTextureAndSampler(1, bindings.normalMap);
}
ResourceBindingPipelineState ResourceBindingCreateInfo::init() {
    return ResourceBindingPipelineState {};
}
}}