#pragma once
#include <SDL2/SDL.h>
#include "spider.h"
#include "types.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(SDL_Window* window);
    void render(const Spider& spider);
    void present();
    void clear();

private:
    void drawSpiderBody(const Spider& spider);
    void drawSpiderLegs(const Spider& spider);
    void drawFilledCircle(int cx, int cy, int radius, Color color);
    void drawFilledEllipse(int cx, int cy, int rx, int ry, Color color);
    void drawThickLine(const Vec2& start, const Vec2& end, float thickness, Color color);
    void drawEyes(const Spider& spider);

    SDL_Renderer* sdlRenderer = nullptr;
    int windowWidth = 0;
    int windowHeight = 0;
};
