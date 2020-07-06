
#ifndef GAME_ENGINE_CAMERA_H
#define GAME_ENGINE_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Window.h"

class Camera {
private:
    Window& window;
    bool firstMousePos;
    double lastX, lastY;
    float pitch = 0;
    float mouseSensitivity, movementSpeed;

    glm::vec3 position;
    glm::quat orientation;
    glm::mat4 projectionMatrix;
public:
    double offsetX, offsetY;
    Camera(Window &window, float mouseSensitivity, float movementSpeed);

    glm::vec3 getPosition();

    void processInput();

    glm::mat4 calculateViewMatrix();
    glm::mat4 calculateProjectionMatrix();
};


#endif //GAME_ENGINE_CAMERA_H
