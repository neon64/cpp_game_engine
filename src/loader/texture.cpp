
#include "texture.h"

Texture2d loadTexture(OpenGLContext &context, const string &path) {
    int x, y, n;
    unsigned char *data = stbi_load(path.c_str(), &x, &y, &n, 0);
    if (data == nullptr) {
        throw TextureLoadingError(stbi_failure_reason());
    }
    cout << "loaded image from " << path << " " << x << ", " << y << ", " << n << endl;

    auto tex = context.buildTexture2D(DataFormat::R8G8B8_UINT, Dimensions2d(x, y), true);
    context.uploadBaseImage2D(tex, TransferFormat::R8G8B8_UINT, data);

    return tex;
}
