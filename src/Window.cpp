
#include "Window.h"
#include "errors.h"

bool Window::hasInitGLFW = false;

Window::Window() {
    if(!hasInitGLFW) {
        Window::initGLFW();
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    handle = glfwCreateWindow(800, 600, "Test", NULL, NULL);
}

Window::~Window() {
    if(handle != nullptr) {
        glfwDestroyWindow(handle);
    }
}

void Window::initGLFW() {
    glfwInit();
    glfwSetErrorCallback(&handleGLFWError);
}

void Window::swapBuffers() {
    glfwSwapBuffers(handle);
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(handle);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::makeContextCurrent() {
    glfwMakeContextCurrent(handle);
}

Window::Window(Window &&other) : handle(other.handle) {
    handle = nullptr;
}
