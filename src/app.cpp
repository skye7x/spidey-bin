#include "app.h"
#include "platform.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>

App::App() {}

App::~App() {
    shutdown();
}

bool App::init() {
    srand(static_cast<unsigned>(time(nullptr)));

#ifdef PLATFORM_LINUX
    // Force X11 backend — entire platform layer depends on X11 APIs
    // (XFixes, XShape, XChangeProperty, XEmbed tray, ARGB visuals).
    // Without this, SDL3/sdl2-compat may pick Wayland, causing segfaults.
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        fprintf(stderr, "This app requires X11. On Wayland, ensure XWayland is available.\n");
        return false;
    }

    Platform::getScreenSize(screenWidth, screenHeight);

    // Ensure a compositor is running for transparency
    Platform::ensureCompositor();

    // Create window with ARGB visual for true transparency
    window = Platform::createTransparentWindow("Spider Pet", screenWidth, screenHeight);

    if (!window) {
        fprintf(stderr, "Transparent window failed, trying fallback\n");
        window = SDL_CreateWindow(
            "Spider Pet", 0, 0, screenWidth, screenHeight,
            SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
        );
    }

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    if (!Platform::setupTransparentWindow(window)) {
        fprintf(stderr, "Warning: Could not set up transparent window\n");
    }

    Platform::hideFromTaskbar(window);

    if (alwaysOnTop) {
        Platform::setAlwaysOnTop(window, true);
    }

    Platform::setClickThrough(window, true);

    if (!renderer.init(window)) {
        fprintf(stderr, "Renderer init failed\n");
        return false;
    }

    // Initialize spider at center of screen
    spider.setPosition(Vec2(screenWidth / 2.0f, screenHeight / 2.0f));

    // Setup tray
    TrayCallbacks callbacks;
    callbacks.onToggleActive = [this]() {
        active = !active;
        tray.setActive(active);
    };
    callbacks.onToggleAggressive = [this]() {
        bool agg = !spider.isAggressive();
        spider.setAggressiveMode(agg);
        tray.setAggressive(agg);
    };
    callbacks.onToggleAlwaysOnTop = [this]() {
        alwaysOnTop = !alwaysOnTop;
        Platform::setAlwaysOnTop(window, alwaysOnTop);
        tray.setAlwaysOnTop(alwaysOnTop);
    };
    callbacks.onExit = [this]() {
        running = false;
    };

    if (!tray.init(callbacks)) {
        fprintf(stderr, "Warning: System tray not available\n");
    }

    lastFrameTime = SDL_GetPerformanceCounter();

    return true;
}

void App::run() {
    while (running) {
        uint64_t now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - lastFrameTime) /
                   static_cast<float>(SDL_GetPerformanceFrequency());
        lastFrameTime = now;

        // Cap delta time to avoid physics explosions
        if (dt > 0.05f) dt = 0.05f;

        handleEvents();

        if (active) {
            update(dt);
        }

        render();

        // Target ~60 FPS
        SDL_Delay(1);
    }
}

void App::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
                break;
            default:
                break;
        }
    }

    tray.update();
}

void App::update(float dt) {
    int mx, my;
    Platform::getCursorPosition(mx, my);
    Vec2 cursorPos(static_cast<float>(mx), static_cast<float>(my));

    spider.update(dt, cursorPos, screenWidth, screenHeight);
}

void App::render() {
    renderer.clear();
    renderer.render(spider);
    renderer.present();
}

void App::shutdown() {
    tray.shutdown();

    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    Platform::shutdown();
    SDL_Quit();
}
