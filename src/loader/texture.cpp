
#define LOGURU_WUTH_STREAMS 1
#include <loguru/loguru.hpp>
#include <algorithm>
#include "texture.h"
#include "stb_image.h"

Texture2d create1By1Texture(OpenGLContext &context, glm::vec3 color) {
    auto tex = context.buildTexture2D(DataFormat::R8G8B8_UINT, Dimensions2d(1, 1), false);

    uint8_t data[3] = {
        (uint8_t) std::clamp(color.x * 255.0f, 0.0f, 255.0f),
        (uint8_t) std::clamp(color.y * 255.0f, 0.0f, 255.0f),
        (uint8_t) std::clamp(color.z * 255.0f, 0.0f, 255.0f)
    };

    context.uploadBaseImage2D(tex, TransferFormat::R8G8B8_UINT, (const void *) &data);

    return std::move(tex);
}

Texture2d create1By1NormalMap(OpenGLContext &context, glm::vec3 normal) {
    return std::move(create1By1Texture(context, (normal + glm::vec3(1.0f)) * 0.5f));
}

shared_ptr<Texture2d> Texture2dMetadata::build(OpenGLContext &context) {
    int x, y, n;

    int numComponents;
    if(format == DesiredTextureFormat::DONT_CARE) {
        numComponents = 0;
    } else if(format == DesiredTextureFormat::ALPHA) {
        numComponents = 4;
    } else if(format == DesiredTextureFormat::NO_ALPHA) {
        numComponents = 3;
    }

    unsigned char *data = stbi_load(path.c_str(), &x, &y, &n, numComponents);
    if (data == nullptr) {
        throw TextureLoadingError(stbi_failure_reason());
    }
    LOG_S(INFO) << "loaded image from " << path << " " << x << ", " << y << ", " << n;

    int actualComponents = numComponents;
    if(actualComponents == 0) {
        actualComponents = n;
    }

    DataFormat dformat;
    TransferFormat tformat;
    if(actualComponents == 3) {
        dformat = DataFormat::R8G8B8_UINT;
        tformat = TransferFormat::R8G8B8_UINT;
    } else if(actualComponents == 4) {
        dformat = DataFormat::R8G8B8A8_UINT;
        tformat = TransferFormat::R8G8B8A8_UINT;
    } else {
        LOG_S(ERROR) << "unsupported number of components in image: " << actualComponents;
        assert(false);
    }

    auto tex = context.buildTexture2D(dformat, Dimensions2d(x, y), true);
    context.uploadBaseImage2D(tex, tformat, data);

    return make_shared<Texture2d>(std::move(tex));
}
