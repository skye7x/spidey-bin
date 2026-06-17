#include "tray.h"
#include <cstdio>

// ============================================================================
// Windows tray implementation: Shell_NotifyIcon + hidden message window
// ============================================================================
#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON     (WM_USER + 1)
#define IDM_TOGGLE_ACTIVE     1001
#define IDM_TOGGLE_AGGRESSIVE 1002
#define IDM_TOGGLE_ONTOP      1003
#define IDM_EXIT              1004

// Data shared between Tray methods and the WndProc callback
struct TrayData {
    TrayCallbacks callbacks;
    bool active = true;
    bool aggressive = false;
    bool alwaysOnTop = true;
    bool initialized = false;

    HWND messageWindow = nullptr;
    NOTIFYICONDATAW nid = {};
    ATOM wndClassAtom = 0;

    void updateTooltip() {
        const wchar_t* stateStr = active ? L"Running" : L"Paused";
        const wchar_t* modeStr = aggressive ? L" [Aggressive]" : L"";
        swprintf(nid.szTip, sizeof(nid.szTip) / sizeof(wchar_t),
                 L"Spider Pet - %ls%ls", stateStr, modeStr);
        if (initialized) {
            Shell_NotifyIconW(NIM_MODIFY, &nid);
        }
    }
};

struct Tray::Impl {
    TrayData data;
};

static LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    TrayData* td = reinterpret_cast<TrayData*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            td = reinterpret_cast<TrayData*>(cs->lpCreateParams);
            return 0;
        }
        case WM_TRAYICON: {
            if (!td) break;
            if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_LBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);

                HMENU menu = CreatePopupMenu();
                AppendMenuW(menu,
                    MF_STRING | (td->active ? MF_CHECKED : MF_UNCHECKED),
                    IDM_TOGGLE_ACTIVE, L"Spider Active");
                AppendMenuW(menu,
                    MF_STRING | (td->aggressive ? MF_CHECKED : MF_UNCHECKED),
                    IDM_TOGGLE_AGGRESSIVE, L"Aggressive Mode");
                AppendMenuW(menu,
                    MF_STRING | (td->alwaysOnTop ? MF_CHECKED : MF_UNCHECKED),
                    IDM_TOGGLE_ONTOP, L"Always On Top");
                AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(menu, MF_STRING, IDM_EXIT, L"Exit");

                TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
                DestroyMenu(menu);
                PostMessageW(hwnd, WM_NULL, 0, 0);
            }
            return 0;
        }
        case WM_COMMAND: {
            if (!td) break;
            switch (LOWORD(wParam)) {
                case IDM_TOGGLE_ACTIVE:
                    if (td->callbacks.onToggleActive) td->callbacks.onToggleActive();
                    break;
                case IDM_TOGGLE_AGGRESSIVE:
                    if (td->callbacks.onToggleAggressive) td->callbacks.onToggleAggressive();
                    break;
                case IDM_TOGGLE_ONTOP:
                    if (td->callbacks.onToggleAlwaysOnTop) td->callbacks.onToggleAlwaysOnTop();
                    break;
                case IDM_EXIT:
                    if (td->callbacks.onExit) td->callbacks.onExit();
                    break;
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static HICON createSpiderIcon() {
    const int size = 16;
    HDC screenDC = GetDC(nullptr);
    HDC colorDC = CreateCompatibleDC(screenDC);
    HDC maskDC = CreateCompatibleDC(screenDC);
    HBITMAP colorBmp = CreateCompatibleBitmap(screenDC, size, size);
    HBITMAP maskBmp = CreateBitmap(size, size, 1, 1, nullptr);
    ReleaseDC(nullptr, screenDC);

    SelectObject(colorDC, colorBmp);
    SelectObject(maskDC, maskBmp);

    // Fill mask white (transparent) and color black
    RECT rc = {0, 0, size, size};
    HBRUSH whiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    FillRect(maskDC, &rc, whiteBrush);
    FillRect(colorDC, &rc, blackBrush);

    // Draw spider body (dark red/brown circle) in center
    HBRUSH bodyBrush = CreateSolidBrush(RGB(60, 30, 20));
    HBRUSH eyeBrush = CreateSolidBrush(RGB(200, 30, 20));
    HPEN legPen = CreatePen(PS_SOLID, 1, RGB(50, 35, 25));

    SelectObject(colorDC, bodyBrush);
    SelectObject(maskDC, blackBrush);

    // Body (make visible in mask too)
    Ellipse(colorDC, 5, 5, 11, 11);
    Ellipse(maskDC, 5, 5, 11, 11);

    // Legs
    SelectObject(colorDC, legPen);
    HPEN maskPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    SelectObject(maskDC, maskPen);

    int cx = 8, cy = 8;
    int legEnds[][2] = {
        {2, 2}, {1, 6}, {2, 10}, {3, 14},
        {14, 2}, {15, 6}, {14, 10}, {13, 14}
    };
    for (auto& le : legEnds) {
        MoveToEx(colorDC, cx, cy, nullptr);
        LineTo(colorDC, le[0], le[1]);
        MoveToEx(maskDC, cx, cy, nullptr);
        LineTo(maskDC, le[0], le[1]);
    }

    // Eyes
    SelectObject(colorDC, eyeBrush);
    SetPixel(colorDC, 7, 6, RGB(200, 30, 20));
    SetPixel(colorDC, 9, 6, RGB(200, 30, 20));

    DeleteObject(bodyBrush);
    DeleteObject(eyeBrush);
    DeleteObject(legPen);
    DeleteObject(maskPen);
    DeleteDC(colorDC);
    DeleteDC(maskDC);

    // In mask bitmap: 0 = opaque, 1 = transparent (inverted from what we drew)
    // We drew the spider shape as black (0) on white (1) mask — correct

    ICONINFO iconInfo = {};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmMask = maskBmp;
    iconInfo.hbmColor = colorBmp;
    HICON icon = CreateIconIndirect(&iconInfo);

    DeleteObject(colorBmp);
    DeleteObject(maskBmp);
    return icon;
}

Tray::Tray() {}
Tray::~Tray() { shutdown(); }

bool Tray::init(const TrayCallbacks& callbacks) {
    impl = new Impl();
    TrayData& d = impl->data;
    d.callbacks = callbacks;

    // Register window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"SpiderPetTray";
    d.wndClassAtom = RegisterClassW(&wc);
    if (!d.wndClassAtom) {
        fprintf(stderr, "Failed to register tray window class\n");
        delete impl;
        impl = nullptr;
        return false;
    }

    // Create message-only window, passing TrayData* as lpCreateParams
    d.messageWindow = CreateWindowExW(
        0, L"SpiderPetTray", L"SpiderPetTray",
        0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
        GetModuleHandleW(nullptr), &d);

    if (!d.messageWindow) {
        fprintf(stderr, "Failed to create tray message window\n");
        delete impl;
        impl = nullptr;
        return false;
    }

    // Create tray icon
    d.nid.cbSize = sizeof(NOTIFYICONDATAW);
    d.nid.hWnd = d.messageWindow;
    d.nid.uID = 1;
    d.nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    d.nid.uCallbackMessage = WM_TRAYICON;
    d.nid.hIcon = createSpiderIcon();
    wcscpy_s(d.nid.szTip, L"Spider Pet - Running");

    if (!Shell_NotifyIconW(NIM_ADD, &d.nid)) {
        fprintf(stderr, "Shell_NotifyIcon failed\n");
        DestroyWindow(d.messageWindow);
        delete impl;
        impl = nullptr;
        return false;
    }

    d.initialized = true;
    return true;
}

void Tray::update() {
    if (!impl || !impl->data.initialized) return;

    // Pump messages for the hidden tray window
    MSG msg;
    while (PeekMessageW(&msg, impl->data.messageWindow, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void Tray::shutdown() {
    if (impl) {
        TrayData& d = impl->data;
        if (d.initialized) {
            Shell_NotifyIconW(NIM_DELETE, &d.nid);
            if (d.nid.hIcon) DestroyIcon(d.nid.hIcon);
        }
        if (d.messageWindow) DestroyWindow(d.messageWindow);
        if (d.wndClassAtom) {
            UnregisterClassW(L"SpiderPetTray", GetModuleHandleW(nullptr));
        }
        d.initialized = false;
        delete impl;
        impl = nullptr;
    }
}

void Tray::setActive(bool active) {
    if (impl) {
        impl->data.active = active;
        impl->data.updateTooltip();
    }
}

void Tray::setAggressive(bool aggressive) {
    if (impl) {
        impl->data.aggressive = aggressive;
        impl->data.updateTooltip();
    }
}

void Tray::setAlwaysOnTop(bool onTop) {
    if (impl) impl->data.alwaysOnTop = onTop;
}

// ============================================================================
// Linux tray implementation: X11 system tray (freedesktop.org XEmbed protocol)
// ============================================================================
#elif defined(PLATFORM_LINUX)

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>

#define SYSTEM_TRAY_REQUEST_DOCK   0
#define SYSTEM_TRAY_BEGIN_MESSAGE  1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

struct MenuItem {
    std::string label;
    int id;
    bool checked;
};

struct Tray::Impl {
    TrayCallbacks callbacks;
    bool active = true;
    bool aggressive = false;
    bool alwaysOnTop = true;
    bool initialized = false;

    Display* display = nullptr;
    Window iconWindow = 0;
    Window menuWindow = 0;
    GC gc = nullptr;
    int iconSize = 24;
    bool menuVisible = false;

    void drawIcon() {
        if (!display || !iconWindow) return;

        // Get actual window size
        XWindowAttributes attrs;
        XGetWindowAttributes(display, iconWindow, &attrs);
        int w = attrs.width;
        int h = attrs.height;

        // Clear
        XSetForeground(display, gc, 0x202020);
        XFillRectangle(display, iconWindow, gc, 0, 0, w, h);

        int cx = w / 2, cy = h / 2;
        int bodyR = std::min(w, h) / 5;

        // Body
        XSetForeground(display, gc, 0x3C1E14);
        XFillArc(display, iconWindow, gc,
                 cx - bodyR, cy - bodyR, bodyR * 2, bodyR * 2, 0, 360 * 64);

        // Legs
        XSetForeground(display, gc, 0x322319);
        int legLen = bodyR * 3;
        float angles[] = {-2.4f, -1.6f, -0.8f, -0.3f, 0.3f, 0.8f, 1.6f, 2.4f};
        for (float a : angles) {
            int ex = cx + static_cast<int>(std::cos(a) * legLen);
            int ey = cy + static_cast<int>(std::sin(a) * legLen);
            XDrawLine(display, iconWindow, gc, cx, cy, ex, ey);
        }

        // Eyes
        XSetForeground(display, gc, 0xC81E14);
        XFillArc(display, iconWindow, gc, cx - 3, cy - bodyR + 1, 3, 3, 0, 360 * 64);
        XFillArc(display, iconWindow, gc, cx + 1, cy - bodyR + 1, 3, 3, 0, 360 * 64);

        XFlush(display);
    }

    void showMenu(int rootX, int rootY) {
        if (menuVisible) hideMenu();

        std::vector<MenuItem> items = {
            {"Spider Active", 1, active},
            {"Aggressive Mode", 2, aggressive},
            {"Always On Top", 3, alwaysOnTop},
            {"", 0, false},  // separator
            {"Exit", 4, false}
        };

        int itemH = 22;
        int menuW = 180;
        int menuH = 0;
        for (auto& item : items) {
            menuH += item.label.empty() ? 6 : itemH;
        }

        int screen = DefaultScreen(display);
        int screenW = DisplayWidth(display, screen);
        int screenH = DisplayHeight(display, screen);

        int mx = rootX;
        int my = rootY - menuH;
        if (mx + menuW > screenW) mx = screenW - menuW;
        if (my < 0) my = rootY;

        menuWindow = XCreateSimpleWindow(display,
            RootWindow(display, screen),
            mx, my, menuW, menuH, 1, 0x444444, 0x2a2a2a);

        XSetWindowAttributes swa;
        swa.override_redirect = True;
        swa.save_under = True;
        XChangeWindowAttributes(display, menuWindow,
            CWOverrideRedirect | CWSaveUnder, &swa);

        XSelectInput(display, menuWindow,
            ExposureMask | ButtonPressMask | ButtonReleaseMask |
            PointerMotionMask | LeaveWindowMask);

        XMapRaised(display, menuWindow);
        XFlush(display);

        // Grab pointer to detect clicks outside
        XGrabPointer(display, menuWindow, True,
            ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
            GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

        menuVisible = true;
    }

    void hideMenu() {
        if (!menuVisible) return;
        XUngrabPointer(display, CurrentTime);
        if (menuWindow) {
            XDestroyWindow(display, menuWindow);
            menuWindow = 0;
        }
        menuVisible = false;
        XFlush(display);
    }

    void drawMenu() {
        if (!menuWindow) return;

        std::vector<MenuItem> items = {
            {"Spider Active", 1, active},
            {"Aggressive Mode", 2, aggressive},
            {"Always On Top", 3, alwaysOnTop},
            {"", 0, false},
            {"Exit", 4, false}
        };

        int y = 0;
        int itemH = 22;
        int menuW = 180;

        for (auto& item : items) {
            if (item.label.empty()) {
                XSetForeground(display, gc, 0x444444);
                XDrawLine(display, menuWindow, gc, 4, y + 3, menuW - 4, y + 3);
                y += 6;
            } else {
                XSetForeground(display, gc, 0xdddddd);
                std::string text = item.label;
                if (item.checked) text = "[x] " + text;
                else if (item.id <= 3) text = "[ ] " + text;
                XDrawString(display, menuWindow, gc, 8, y + 16,
                           text.c_str(), static_cast<int>(text.length()));
                y += itemH;
            }
        }
        XFlush(display);
    }

    int hitTestMenu(int my) {
        int y = 0;
        int itemH = 22;
        int items[] = {1, 2, 3, 0, 4}; // ids, 0=separator
        int heights[] = {itemH, itemH, itemH, 6, itemH};

        for (int i = 0; i < 5; i++) {
            if (my >= y && my < y + heights[i]) {
                return items[i];
            }
            y += heights[i];
        }
        return -1;
    }

    void handleMenuAction(int id) {
        switch (id) {
            case 1: if (callbacks.onToggleActive) callbacks.onToggleActive(); break;
            case 2: if (callbacks.onToggleAggressive) callbacks.onToggleAggressive(); break;
            case 3: if (callbacks.onToggleAlwaysOnTop) callbacks.onToggleAlwaysOnTop(); break;
            case 4: if (callbacks.onExit) callbacks.onExit(); break;
        }
    }
};

Tray::Tray() {}
Tray::~Tray() { shutdown(); }

bool Tray::init(const TrayCallbacks& callbacks) {
    impl = new Impl();
    impl->callbacks = callbacks;

    impl->display = XOpenDisplay(nullptr);
    if (!impl->display) {
        fprintf(stderr, "Tray: cannot open X display\n");
        return false;
    }

    int screen = DefaultScreen(impl->display);

    // Find the system tray manager
    char atomName[64];
    snprintf(atomName, sizeof(atomName), "_NET_SYSTEM_TRAY_S%d", screen);
    Atom trayAtom = XInternAtom(impl->display, atomName, False);
    Window trayManager = XGetSelectionOwner(impl->display, trayAtom);

    if (trayManager == None) {
        fprintf(stderr, "Tray: no system tray manager found\n");
        XCloseDisplay(impl->display);
        impl->display = nullptr;
        delete impl;
        impl = nullptr;
        return false;
    }

    // Create icon window
    impl->iconWindow = XCreateSimpleWindow(impl->display,
        RootWindow(impl->display, screen),
        0, 0, impl->iconSize, impl->iconSize, 0, 0, 0x202020);

    XSelectInput(impl->display, impl->iconWindow,
        ExposureMask | ButtonPressMask | StructureNotifyMask);

    // Request docking
    Atom opcodeAtom = XInternAtom(impl->display, "_NET_SYSTEM_TRAY_OPCODE", False);
    XEvent ev = {};
    ev.xclient.type = ClientMessage;
    ev.xclient.window = trayManager;
    ev.xclient.message_type = opcodeAtom;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
    ev.xclient.data.l[2] = static_cast<long>(impl->iconWindow);
    XSendEvent(impl->display, trayManager, False, NoEventMask, &ev);

    impl->gc = XCreateGC(impl->display, impl->iconWindow, 0, nullptr);
    XSync(impl->display, False);

    impl->initialized = true;
    return true;
}

void Tray::update() {
    if (!impl || !impl->display) return;

    while (XPending(impl->display)) {
        XEvent ev;
        XNextEvent(impl->display, &ev);

        if (ev.type == Expose) {
            if (ev.xexpose.window == impl->iconWindow) {
                impl->drawIcon();
            } else if (ev.xexpose.window == impl->menuWindow) {
                impl->drawMenu();
            }
        } else if (ev.type == ButtonPress) {
            if (ev.xbutton.window == impl->iconWindow) {
                // Get icon position in root coordinates
                Window child;
                int rootX, rootY;
                XTranslateCoordinates(impl->display, impl->iconWindow,
                    DefaultRootWindow(impl->display),
                    ev.xbutton.x, ev.xbutton.y, &rootX, &rootY, &child);
                impl->showMenu(rootX, rootY);
            }
        } else if (ev.type == ButtonRelease) {
            if (impl->menuVisible) {
                // Check if click is inside menu
                if (ev.xbutton.window == impl->menuWindow) {
                    int id = impl->hitTestMenu(ev.xbutton.y);
                    if (id > 0) {
                        impl->handleMenuAction(id);
                    }
                }
                impl->hideMenu();
            }
        } else if (ev.type == LeaveNotify && impl->menuVisible) {
            // Menu might dismiss on leave in some cases
        } else if (ev.type == ConfigureNotify) {
            if (ev.xconfigure.window == impl->iconWindow) {
                impl->drawIcon();
            }
        }
    }
}

void Tray::shutdown() {
    if (impl) {
        if (impl->menuVisible) impl->hideMenu();
        if (impl->gc) XFreeGC(impl->display, impl->gc);
        if (impl->iconWindow && impl->display) {
            XDestroyWindow(impl->display, impl->iconWindow);
        }
        if (impl->display) {
            XCloseDisplay(impl->display);
        }
        impl->initialized = false;
        delete impl;
        impl = nullptr;
    }
}

void Tray::setActive(bool active) {
    if (impl) {
        impl->active = active;
        impl->drawIcon();
    }
}

void Tray::setAggressive(bool aggressive) {
    if (impl) {
        impl->aggressive = aggressive;
        impl->drawIcon();
    }
}

void Tray::setAlwaysOnTop(bool onTop) {
    if (impl) impl->alwaysOnTop = onTop;
}

// ============================================================================
// Fallback: no-op tray when no platform tray is available
// ============================================================================
#else

struct Tray::Impl {
    TrayCallbacks callbacks;
    bool initialized = false;
};

Tray::Tray() {}
Tray::~Tray() { shutdown(); }

bool Tray::init(const TrayCallbacks& callbacks) {
    impl = new Impl();
    impl->callbacks = callbacks;
    impl->initialized = true;
    fprintf(stdout, "Spider Pet running (no tray support on this platform).\n");
    return true;
}

void Tray::update() {}

void Tray::shutdown() {
    delete impl;
    impl = nullptr;
}

void Tray::setActive(bool) {}
void Tray::setAggressive(bool) {}
void Tray::setAlwaysOnTop(bool) {}

#endif
