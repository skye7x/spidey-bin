#pragma once
#include <SDL2/SDL.h>

namespace Platform {
    // Create a window with ARGB transparency support
    SDL_Window* createTransparentWindow(const char* title, int w, int h);

    // Configure window properties after creation
    bool setupTransparentWindow(SDL_Window* window);

    // Set window always on top
    void setAlwaysOnTop(SDL_Window* window, bool onTop);

    // Hide from taskbar
    void hideFromTaskbar(SDL_Window* window);

    // Get screen dimensions
    void getScreenSize(int& width, int& height);

    // Get cursor position (global)
    void getCursorPosition(int& x, int& y);

    // Get DPI scale factor
    float getDPIScale();

    // Set click-through (mouse passes through window)
    void setClickThrough(SDL_Window* window, bool clickThrough);

    // Start compositor if needed (Linux)
    void ensureCompositor();

    // Release cached platform resources
    void shutdown();
}
