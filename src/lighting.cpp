
#include "lighting.h"

void DirectionalLight::set(pipelines::lighting_test::Light *light) {
    light->position = glm::vec3(direction);
    light->range = -1.0f;
    light->color = color;
    light->direction = glm::vec3(0.0);
    light->innerConeAngle = 0.0f;
    light->outerConeAngle = 0.0f;
    light->attenuation = glm::vec3(0.0f);
    light->ambientCoefficient = ambientCoefficient;
}

DirectionalLight::DirectionalLight(glm::vec3 direction, glm::vec3 color, float ambientCoefficient) : BaseLight(direction, color, ambientCoefficient), direction(direction) {}

void BaseLight::setAmbientCoefficient(float ambient) {
    ambientCoefficient = ambient;
}

BaseLight::BaseLight(glm::vec3 position, glm::vec3 color, float ambientCoefficient) : position(position), color(color), ambientCoefficient(ambientCoefficient) {}

PointLight::PointLight(glm::vec3 position, glm::vec3 color, Attenuation attenuation) : BaseLight(position, color, 0), attenuation(attenuation) {
    // calculates the maximum range of the point light https://community.khronos.org/t/max-range-of-point-light/25202
    float a = attenuation.exponent;
    float b = attenuation.linear;
    float c = attenuation.constant - COLOR_DEPTH * glm::length(color) * glm::compMax(color);

    range = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
}
