
#ifndef GAME_ENGINE_TEXTURE_H
#define GAME_ENGINE_TEXTURE_H

#include <string>
#include <exception>
#include "../texturing.h"
#include "../OpenGLContext.h"
#include "stb_image.h"

using namespace std;

class TextureLoadingError : exception {
public:
    const char *stbi_reason;
    TextureLoadingError(const char* stbi_reason) : stbi_reason(stbi_reason) {}
    ~TextureLoadingError() throw () {} // Updated
    const char* what() const throw() { return stbi_reason; }
};

Texture2d loadTexture(OpenGLContext &context, const std::string& path);


#endif //GAME_ENGINE_TEXTURE_H
