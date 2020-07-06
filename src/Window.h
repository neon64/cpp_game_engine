#ifndef GAME_ENGINE_WINDOW_H
#define GAME_ENGINE_WINDOW_H

#include <memory>
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include "util.h"
#include <GLFW/glfw3.h>

class Window {
private:
    static bool hasInitGLFW;
    static void initGLFW();

    Dimensions2d windowedSize;
    Dimensions2d size;

    std::vector<std::function<void(Dimensions2d)>> resizeCallbacks;

public:
    GLFWwindow* handle;

    Window(Dimensions2d size, const char *name);

    // can only move, not copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // immovable type
//    Window(Window&& other);
//    operator=(Window&& other);

    std::function<void(int, int, int, int)> keyCallback;
    std::function<void(double, double)> cursorPosCallback;

    void setKeyCallback(std::function<void(int, int, int, int)> keyCallback);
    void setCursorPosCallback(std::function<void(double, double)> cursorPosCallback);

    void addResizeCallback(std::function<void(Dimensions2d)> resizeCallback);

    void makeContextCurrent();

    void grabMouseCursor();
    void releaseMouseCursor();

    Dimensions2d getSize();

    bool isKeyDown(int key) const;

    bool shouldClose() const;
    void swapBuffers();

    void onResize(Dimensions2d newSize);

    void enterFullscreen();
    bool isFullscreen();
    void exitFullscreen();
    void toggleFullscreen();

    static void pollEvents();
    static void terminate();

    ~Window();
};


#endif //GAME_ENGINE_WINDOW_H
