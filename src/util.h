
#ifndef GAME_ENGINE_UTIL_H
#define GAME_ENGINE_UTIL_H

#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_access.hpp>

inline glm::vec3 rotatePoint(glm::quat q, glm::vec3 point) {
    return q * point;
}

namespace glsl {
    struct mat3 {
        alignas(16) glm::vec3 column0;
        alignas(16) glm::vec3 column1;
        alignas(16) glm::vec3 column2;

        mat3(glm::mat3 source) {
            column0 = glm::column(source, 0);
            column1 = glm::column(source, 1);
            column2 = glm::column(source, 2);
        }
    };

    struct mat4 {
        alignas(16) glm::vec4 column0;
        alignas(16) glm::vec4 column1;
        alignas(16) glm::vec4 column2;
        alignas(16) glm::vec4 column3;

        mat4(glm::mat4 source) {
            column0 = glm::column(source, 0);
            column1 = glm::column(source, 1);
            column2 = glm::column(source, 2);
            column3 = glm::column(source, 3);
        }
    };
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
