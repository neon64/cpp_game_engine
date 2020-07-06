
#ifndef GAME_ENGINE_UTIL_H
#define GAME_ENGINE_UTIL_H

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

inline glm::vec3 rotatePoint(glm::quat q, glm::vec3 point) {
    return q * point;
}

struct Point2d {
    uint32_t x;
    uint32_t y;

    Point2d(uint32_t x, uint32_t y) : x(x), y(x) {}
};

struct Dimensions2d {
    uint32_t width;
    uint32_t height;

    Dimensions2d(uint32_t width, uint32_t height) : width(width), height(height) {}

    Dimensions2d min(Dimensions2d other) {
        return Dimensions2d(width > other.width ? other.width : width, height > other.height ? other.height : height);
    }

    Dimensions2d reduceSize(int factor) {
        return Dimensions2d(width >> factor, height >> factor);
    }
};

struct Rect2d {
    Point2d origin;
    Dimensions2d size;

    Rect2d(Point2d origin, Dimensions2d size) : origin(origin), size(size) {}

    static Rect2d fromOrigin(uint32_t width, uint32_t height) {
        return Rect2d(Point2d(0, 0), Dimensions2d(width, height));
    }
};

#endif //GAME_ENGINE_UTIL_H
