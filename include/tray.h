#pragma once
#include <functional>
#include <string>

struct TrayCallbacks {
    std::function<void()> onToggleActive;
    std::function<void()> onToggleAggressive;
    std::function<void()> onToggleAlwaysOnTop;
    std::function<void()> onExit;
};

class Tray {
public:
    Tray();
    ~Tray();

    bool init(const TrayCallbacks& callbacks);
    void update();
    void shutdown();

    void setActive(bool active);
    void setAggressive(bool aggressive);
    void setAlwaysOnTop(bool onTop);

private:
    struct Impl;
    Impl* impl = nullptr;
};
