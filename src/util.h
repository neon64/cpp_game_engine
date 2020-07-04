
#ifndef GAME_ENGINE_UTIL_H
#define GAME_ENGINE_UTIL_H

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

inline glm::vec3 rotatePoint(glm::quat q, glm::vec3 point) {
    return q * point;
}

struct Dimensions2d {
    uint32_t width;
    uint32_t height;

    Dimensions2d(uint32_t width, uint32_t height) : width(width), height(height) {}
};

#endif //GAME_ENGINE_UTIL_H
