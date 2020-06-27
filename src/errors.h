#ifndef GAME_ENGINE_ERRORS_H
#define GAME_ENGINE_ERRORS_H

void handleGLFWError(int code, const char* desc);

void handleGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

#endif //GAME_ENGINE_ERRORS_H
