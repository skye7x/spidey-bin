#include "platform.h"

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <SDL2/SDL_syswm.h>
#include <cstdio>

namespace Platform {

void ensureCompositor() {
    // DWM is always running on modern Windows (8+)
}

SDL_Window* createTransparentWindow(const char* title, int w, int h) {
    return SDL_CreateWindow(title, 0, 0, w, h,
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
}

bool setupTransparentWindow(SDL_Window* window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);

    if (!SDL_GetWindowWMInfo(window, &wmInfo)) {
        fprintf(stderr, "Could not get WM info: %s\n", SDL_GetError());
        return false;
    }

    HWND hwnd = wmInfo.info.win.window;

    // Tool window hides from taskbar; transparent allows click-through
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT;
    exStyle &= ~WS_EX_APPWINDOW;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

    // Color-key transparency: magenta pixels become fully transparent,
    // revealing ALL windows below (not just the desktop wallpaper).
    // DWM glass (LWA_ALPHA + DwmExtendFrameIntoClientArea) only composites
    // against the desktop, so other apps would not show through.
    SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);

    return true;
}

void setAlwaysOnTop(SDL_Window* window, bool onTop) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo)) return;

    HWND hwnd = wmInfo.info.win.window;
    SetWindowPos(hwnd, onTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void hideFromTaskbar(SDL_Window* window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo)) return;

    HWND hwnd = wmInfo.info.win.window;
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_TOOLWINDOW;
    exStyle &= ~WS_EX_APPWINDOW;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
}

void getScreenSize(int& width, int& height) {
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
}

void getCursorPosition(int& x, int& y) {
    POINT pt;
    GetCursorPos(&pt);
    x = pt.x;
    y = pt.y;
}

float getDPIScale() {
    HDC hdc = GetDC(NULL);
    float dpi = static_cast<float>(GetDeviceCaps(hdc, LOGPIXELSX));
    ReleaseDC(NULL, hdc);
    return dpi / 96.0f;
}

void setClickThrough(SDL_Window* window, bool clickThrough) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo)) return;

    HWND hwnd = wmInfo.info.win.window;
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    if (clickThrough) {
        exStyle |= WS_EX_TRANSPARENT;
    } else {
        exStyle &= ~WS_EX_TRANSPARENT;
    }
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
}

void shutdown() {
    // No cached resources on Windows
}

} // namespace Platform

#endif // PLATFORM_WINDOWS
