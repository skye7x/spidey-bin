#pragma once
#include <SDL2/SDL.h>
#include "spider.h"
#include "renderer.h"
#include "tray.h"

class App {
public:
    App();
    ~App();

    bool init();
    void run();
    void shutdown();

private:
    void handleEvents();
    void update(float dt);
    void render();

    SDL_Window* window = nullptr;
    Renderer renderer;
    Spider spider;
    Tray tray;

    bool running = true;
    bool active = true;
    bool alwaysOnTop = true;
    int screenWidth = 0;
    int screenHeight = 0;

    uint64_t lastFrameTime = 0;
};
