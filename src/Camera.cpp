
#include <iostream>
#include "Camera.h"
#include "Window.h"
#include "util.h"

using namespace std;

Camera::Camera(Window &window, float mouseSensitivity, float movementSpeed) : window(window), firstMousePos(true), mouseSensitivity(mouseSensitivity), movementSpeed(movementSpeed) {
    orientation = glm::lookAt(glm::vec3(0, 0, -1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    position = glm::vec3(0.0f, 0.0f, 1.0f);

    window.setCursorPosCallback([this](double x, double y) {
        if(firstMousePos) {
            lastX = x;
            lastY = y;
            firstMousePos = false;
        }
        offsetX = x - lastX;
        offsetY = y - lastY;
        lastX = x;
        lastY = y;

        pitch += offsetY * this->mouseSensitivity;

        // according to this StackExchange answer:
        // https://gamedev.stackexchange.com/questions/30644/how-to-keep-my-quaternion-using-fps-camera-from-tilting-and-messing-up
        // should use a fixed (1, 0, 0) coord for the pitch, but then multiply in the opposite order.

        glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);

        glm::quat pitch = glm::angleAxis((float) -offsetY * this->mouseSensitivity, right);
        orientation = orientation * pitch;

        glm::quat yaw = glm::angleAxis((float) -offsetX * this->mouseSensitivity, glm::vec3(0.0f, 1.0f, 0.0f));
        orientation = yaw * orientation;
    });

    window.addResizeCallback([this](Dimensions2d newSize) {
        projectionMatrix = glm::perspective(glm::radians(45.0f), (float) newSize.width / (float) newSize.height, 0.01f, 10.f);
    });
}

void Camera::processInput() {
    if(window.isKeyDown(GLFW_KEY_D)) {
        //view = glm::translate(view, glm::vec3(-0.01f, 0.0f, 0.0f));
        position += glm::mat3_cast(orientation) * glm::vec3(movementSpeed, 0.0f, 0.0f);
    }
    if(window.isKeyDown(GLFW_KEY_A)) {
        position += glm::mat3_cast(orientation) * glm::vec3(-movementSpeed, 0.0f, 0.0f);
    }
    if(window.isKeyDown(GLFW_KEY_W)) {
        //position += glm::mat3_cast(orientation) * glm::vec3(0.0f, 0.0f, -movementSpeed);
        position += rotatePoint(orientation, glm::vec3(0.0f, 0.0f, -movementSpeed));
    }
    if(window.isKeyDown(GLFW_KEY_S)) {
        position += glm::mat3_cast(orientation) * glm::vec3(0.0f, 0.0f, movementSpeed);
    }
}

glm::mat4 Camera::calculateViewMatrix() {
    glm::mat4 translate = glm::translate(glm::mat4(1.0), position);
    glm::mat4 rotate = glm::mat4_cast(orientation);
    //glm::mat4 rotate2 = glm::rotate(glm::mat4(1.0f), -pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    // glm::mat4 scale = glm::scale(glm::mat4(1.0), glm::vec3(1.0f, 1.0f, 1.0f));

    return glm::inverse(translate * rotate);
}

glm::mat4 Camera::calculateProjectionMatrix() {
    return projectionMatrix;
}

glm::vec3 Camera::getPosition() {
    return position;
}
