#include "platform.h"

#ifdef PLATFORM_LINUX

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#include <SDL2/SDL_syswm.h>
#include <cstdio>
#include <cstdlib>

namespace Platform {

static Display* cachedDisplay = nullptr;

static Display* getCachedDisplay() {
    if (!cachedDisplay) {
        cachedDisplay = XOpenDisplay(nullptr);
    }
    return cachedDisplay;
}

void ensureCompositor() {
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return;

    char atomName[32];
    snprintf(atomName, sizeof(atomName), "_NET_WM_CM_S%d", DefaultScreen(dpy));
    Atom cmAtom = XInternAtom(dpy, atomName, False);
    Window cmOwner = XGetSelectionOwner(dpy, cmAtom);
    XCloseDisplay(dpy);

    if (cmOwner == None) {
        fprintf(stdout, "No compositor detected, starting picom...\n");
        int ret = system("picom --backend glx --vsync -b 2>/dev/null || "
                         "picom --backend xrender -b 2>/dev/null || "
                         "xcompmgr -c -C -t-5 -l-5 -r4.2 -o.55 &");
        (void)ret;
        SDL_Delay(500);
    }
}

SDL_Window* createTransparentWindow(const char* title, int w, int h) {
    // Find 32-bit ARGB visual and tell SDL to use it via hint
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) {
        fprintf(stderr, "Cannot open X display\n");
        return nullptr;
    }

    int screen = DefaultScreen(dpy);
    XVisualInfo vinfo;
    if (XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo)) {
        char visualIdStr[32];
        snprintf(visualIdStr, sizeof(visualIdStr), "0x%lx", vinfo.visualid);
        SDL_SetHint(SDL_HINT_VIDEO_X11_WINDOW_VISUALID, visualIdStr);
    } else {
        fprintf(stderr, "No 32-bit visual found, transparency may not work\n");
    }
    XCloseDisplay(dpy);

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

    if (wmInfo.subsystem != SDL_SYSWM_X11) {
        fprintf(stderr, "Not an X11 window (subsystem=%d), cannot setup transparency\n",
                wmInfo.subsystem);
        return false;
    }

    Display* display = wmInfo.info.x11.display;
    Window xwindow = wmInfo.info.x11.window;
    if (!display) {
        fprintf(stderr, "X11 display is null from WM info\n");
        return false;
    }

    // Set window type to utility (no taskbar entry, no decoration)
    Atom windowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom typeUtility = XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", False);
    XChangeProperty(display, xwindow, windowType, XA_ATOM, 32,
                    PropModeReplace, (unsigned char*)&typeUtility, 1);

    // Remove window decorations via _MOTIF_WM_HINTS
    struct {
        unsigned long flags;
        unsigned long functions;
        unsigned long decorations;
        long input_mode;
        unsigned long status;
    } motifHints = {2, 0, 0, 0, 0};
    Atom motifAtom = XInternAtom(display, "_MOTIF_WM_HINTS", False);
    XChangeProperty(display, xwindow, motifAtom, motifAtom, 32,
                    PropModeReplace, (unsigned char*)&motifHints, 5);

    // Skip taskbar and pager, set above
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom skipTaskbar = XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
    Atom skipPager = XInternAtom(display, "_NET_WM_STATE_SKIP_PAGER", False);
    Atom above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    Atom states[] = {skipTaskbar, skipPager, above};
    XChangeProperty(display, xwindow, wmState, XA_ATOM, 32,
                    PropModeReplace, (unsigned char*)states, 3);

    XFlush(display);
    return true;
}

void setAlwaysOnTop(SDL_Window* window, bool onTop) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo)) return;
    if (wmInfo.subsystem != SDL_SYSWM_X11) return;

    Display* display = wmInfo.info.x11.display;
    if (!display) return;
    Window xwindow = wmInfo.info.x11.window;

    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom stateAbove = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);

    XEvent event = {};
    event.xclient.type = ClientMessage;
    event.xclient.window = xwindow;
    event.xclient.message_type = wmState;
    event.xclient.format = 32;
    event.xclient.data.l[0] = onTop ? 1 : 0;
    event.xclient.data.l[1] = static_cast<long>(stateAbove);
    event.xclient.data.l[2] = 0;

    XSendEvent(display, DefaultRootWindow(display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XFlush(display);
}

void hideFromTaskbar(SDL_Window* window) {
    (void)window;
}

void getScreenSize(int& width, int& height) {
    Display* display = getCachedDisplay();
    if (display) {
        Screen* screen = DefaultScreenOfDisplay(display);
        width = screen->width;
        height = screen->height;
    } else {
        width = 1920;
        height = 1080;
    }
}

void getCursorPosition(int& x, int& y) {
    Display* display = getCachedDisplay();
    if (display) {
        Window root = DefaultRootWindow(display);
        Window child;
        int rootX, rootY, winX, winY;
        unsigned int mask;
        XQueryPointer(display, root, &root, &child, &rootX, &rootY, &winX, &winY, &mask);
        x = rootX;
        y = rootY;
    }
}

float getDPIScale() {
    Display* display = getCachedDisplay();
    if (display) {
        float dpi = static_cast<float>(DisplayWidth(display, 0)) /
                    (static_cast<float>(DisplayWidthMM(display, 0)) / 25.4f);
        return dpi / 96.0f;
    }
    return 1.0f;
}

void setClickThrough(SDL_Window* window, bool clickThrough) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo)) return;
    if (wmInfo.subsystem != SDL_SYSWM_X11) return;

    Display* display = wmInfo.info.x11.display;
    if (!display) return;
    Window xwindow = wmInfo.info.x11.window;

    if (clickThrough) {
        XserverRegion region = XFixesCreateRegion(display, nullptr, 0);
        XFixesSetWindowShapeRegion(display, xwindow, ShapeInput, 0, 0, region);
        XFixesDestroyRegion(display, region);
    } else {
        XFixesSetWindowShapeRegion(display, xwindow, ShapeInput, 0, 0, None);
    }
    XFlush(display);
}

void shutdown() {
    if (cachedDisplay) {
        XCloseDisplay(cachedDisplay);
        cachedDisplay = nullptr;
    }
}

} // namespace Platform

#endif // PLATFORM_LINUX
