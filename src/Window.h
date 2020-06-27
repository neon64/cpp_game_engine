#ifndef GAME_ENGINE_WINDOW_H
#define GAME_ENGINE_WINDOW_H

#include <GLFW/glfw3.h>
#include <memory>

class Window {
private:
    static bool hasInitGLFW;
    static void initGLFW();

    GLFWwindow* handle;
public:
    Window();

    // can only move, not copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&& other);

    void makeContextCurrent();

    bool shouldClose() const;
    void swapBuffers();

    static void pollEvents();

    ~Window();
};


#endif //GAME_ENGINE_WINDOW_H
