
#ifndef GAME_ENGINE_TEXTURE_H
#define GAME_ENGINE_TEXTURE_H

#include <string>
#include <memory>
#include <exception>
#include "../graphics/texturing.h"
#include "../graphics/OpenGLContext.h"
#include "cache.h"

using namespace std;

enum class DesiredTextureFormat {
    DONT_CARE,
    NO_ALPHA,
    ALPHA
};

class TextureLoadingError : exception {
public:
    const char *stbi_reason;
    TextureLoadingError(const char* stbi_reason) : stbi_reason(stbi_reason) {}
    ~TextureLoadingError() throw () {} // Updated
    const char* what() const throw() { return stbi_reason; }
};

struct Texture2dMetadata {
    string path;
    DesiredTextureFormat format;

    Texture2dMetadata(string path, DesiredTextureFormat format) : path(path), format(format) {};

    Texture2dMetadata getKey() const {
        return *this;
    };

    shared_ptr<Texture2d> build(OpenGLContext& context);

    bool operator==(const Texture2dMetadata& b) const {
        return path == b.path && format == b.format;
    }

};



namespace std {

    template <>
    struct hash<Texture2dMetadata>
    {
        std::size_t operator()(const Texture2dMetadata& k) const
        {
            using std::size_t;
            using std::hash;
            using std::string;

            // Compute individual hash values for first,
            // second and third and combine them using XOR
            // and bit shifting:

            return ((hash<string>()(k.path)
                     ^ (hash<DesiredTextureFormat>()(k.format) << 1)) >> 1);
        }
    };

}

class Texture2dCache : public ResourceCache<Texture2dMetadata, Texture2d> {

};

Texture2d loadTexture(OpenGLContext &context, const char* path, DesiredTextureFormat format);
Texture2d create1By1Texture(OpenGLContext &context, glm::vec3 color);
Texture2d create1By1NormalMap(OpenGLContext &context, glm::vec3 normal);


#endif //GAME_ENGINE_TEXTURE_H
