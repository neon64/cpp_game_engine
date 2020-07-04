
#include "Window.h"
#include "errors.h"
#include "util.h"

#include <iostream>

using namespace std;

bool Window::hasInitGLFW = false;

void handle_key_press(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (static_cast<Window *>(glfwGetWindowUserPointer(window))->keyCallback)(key, scancode, action, mods);
}

void handle_cursor_pos(GLFWwindow* window, double x, double y) {
    (static_cast<Window *>(glfwGetWindowUserPointer(window))->cursorPosCallback)(x, y);
}

void handle_window_resize(GLFWwindow *window, int width, int height) {
    (static_cast<Window *>(glfwGetWindowUserPointer(window))->onResize)(Dimensions2d(width, height));
}

Window::Window(Dimensions2d size, const char *name) : windowedSize(size) {
    if(!hasInitGLFW) {
        Window::initGLFW();
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    handle = glfwCreateWindow(size.width, size.height, name, NULL, NULL);

    glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        cout << "Raw mouse motion" << endl;
        glfwSetInputMode(handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwSetWindowUserPointer(handle, this);
    glfwSetWindowSizeCallback(handle, handle_window_resize);
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

//Window::Window(Window &&other) : handle(other.handle) {
//    other.handle = nullptr;
//}
//
//int Window::operator=(Window &&other) : handle(other.handle) {
//    other.handle = nullptr;
//}


void Window::setKeyCallback(std::function<void(int, int, int, int)> keyCallback) {
    this->keyCallback = keyCallback;
    glfwSetKeyCallback(handle, handle_key_press);
}

bool Window::isKeyDown(int key) const {
    return glfwGetKey(handle, key) == GLFW_PRESS;
}

void Window::setCursorPosCallback(std::function<void(double, double)> cursorPosCallback) {
    this->cursorPosCallback = cursorPosCallback;
    glfwSetCursorPosCallback(handle, handle_cursor_pos);
}

void Window::addResizeCallback(std::function<void(Dimensions2d)> resizeCallback) {
    int width, height;
    glfwGetWindowSize(handle, &width, &height);
    (resizeCallback)(Dimensions2d(width, height));
    resizeCallbacks.push_back(resizeCallback);
}

void Window::onResize(Dimensions2d newSize) {
    for(auto& callback : resizeCallbacks) {
        (callback)(newSize);
    }
}

void Window::enterFullscreen() {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const char *name = glfwGetMonitorName(monitor);
    cout << "Entering fullscreen on window " << name << endl;
    const GLFWvidmode *videomode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(handle, monitor, 0, 0, videomode->width, videomode->height, GLFW_DONT_CARE);
}

bool Window::isFullscreen() {
    return glfwGetWindowMonitor(handle) != nullptr;
}

void Window::exitFullscreen() {
    glfwSetWindowMonitor(handle, nullptr, 0, 0, windowedSize.width, windowedSize.height, GLFW_DONT_CARE);
}

void Window::toggleFullscreen() {
    if(isFullscreen()) {
        exitFullscreen();
    } else {
        enterFullscreen();
    }
}

