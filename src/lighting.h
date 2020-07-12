

#pragma once

#include "../gen/shaders/lighting_test.h"
#include <glm/vec3.hpp>

class BaseLight {
protected:
    glm::vec3 position;
    glm::vec3 color;
    float ambientCoefficient;
public:
    BaseLight(glm::vec3 position, glm::vec3 color, float ambientCoefficient);

    void setAmbientCoefficient(float ambient);
};

class DirectionalLight : public BaseLight {
    glm::vec3 direction;
public:

    DirectionalLight(glm::vec3 direction, glm::vec3 color, float ambientCoefficient);

    void set(pipelines::lighting_test::Light* light);
};

struct Attenuation {
    float constant;
    float linear;
    float exponent;
};

// using non-HDR render targets, 256 should be sufficient (8-bit colour)
// but with floating-point render targets, may need a higher value
const int COLOR_DEPTH = 3000;

class PointLight : public BaseLight {
    Attenuation attenuation;
    float range;

public:
    PointLight(glm::vec3 position, glm::vec3 color, Attenuation attenuation);

    void set(pipelines::lighting_test::Light* light) {
        light->position = position;
        light->color = color;
        light->ambientCoefficient = ambientCoefficient;
        light->attenuation = glm::vec3(attenuation.constant, attenuation.linear, attenuation.exponent);
        light->range = range;
    }
};