

#pragma once

#include <glm/glm.hpp>

class Transform {

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::quat rot = glm::quatLookAt(glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
    bool dirty = true;

public:
    void setPosition(glm::vec3 amount) {
        position = position + amount;
        dirty = true;
    }

    void setScale(glm::vec3 newScale) {
        scale = newScale;
    }

    void setOrientation(glm::quat newOrientation) {
        rot = newOrientation;
    }

    glm::quat getOrientation() const {
        return rot;
    }

    bool isDirty() {
        bool d = dirty;
        dirty = false;
        return d;
    }

    glm::mat4 getModelMatrix() {
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, scale);
        model = glm::toMat4(rot) * model;
        model = translate * model;
        return model;
    }

    glm::mat3 getNormalMatrix() {
        return glm::mat3(glm::transpose(glm::inverse(getModelMatrix())));
    }

};