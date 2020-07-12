
#ifndef GAME_ENGINE_COLORRGBA_H
#define GAME_ENGINE_COLORRGBA_H


struct ColorRGBA {
    float r;
    float g;
    float b;
    float a;

    ColorRGBA(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}

    bool operator==(const ColorRGBA& other) {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
};


#endif //GAME_ENGINE_COLORRGBA_H
