# Spider Pet

A cross-platform desktop pet application featuring a realistic animated spider that lives on your screen. The spider autonomously wanders, reacts to your cursor, stalks it, and occasionally charges at it.

## Features

- **Transparent overlay** — borderless, always-on-top window with true per-pixel alpha
- **AI state machine** — IDLE, ALERT, STALK, CHARGE, RETREAT, OBSERVE behaviors
- **Steering behaviors** — seek, flee, wander with smooth interpolation
- **Procedural animation** — 8 legs with realistic alternating gait, body breathing, body bob, micro-twitches
- **Click-through** — mouse passes through the spider, never blocking your work
- **System tray icon** — right-click for toggle active, aggressive mode, always-on-top, exit
- **Cross-platform** — Windows (DWM compositing) and Linux (X11 ARGB visual + compositor)
- **60 FPS** with minimal CPU usage
- **DPI aware**

## Requirements

### Linux
```bash
sudo apt-get install build-essential cmake pkg-config \
    libsdl2-dev libx11-dev libxfixes-dev libxext-dev
```

### Windows (MSYS2/MinGW)
```bash
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2
```

### Windows (vcpkg)
```bash
vcpkg install sdl2:x64-windows
```

## Building

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Windows (Visual Studio)
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

## Running

```bash
./build/spider-pet
```

The spider will appear on your screen and begin wandering autonomously.

## Controls

The application is controlled via the **system tray icon**:

| Menu Item | Action |
|-----------|--------|
| Spider Active | Toggle spider on/off (pause movement) |
| Aggressive Mode | Spider charges at cursor more often |
| Always On Top | Keep spider above all windows |
| Exit | Quit the application |

Right-click (or left-click) the spider icon in the system tray to access the menu.

## Behavior States

| State | Description |
|-------|-------------|
| **IDLE** | Random wandering across the screen |
| **ALERT** | Detects cursor nearby, stops and watches |
| **STALK** | Slowly approaches the cursor |
| **CHARGE** | Fast rush toward the cursor |
| **RETREAT** | Quick escape away from cursor |
| **OBSERVE** | Stops and watches from a distance |

## Architecture

```
src/
├── main.cpp           # Entry point
├── app.cpp            # Application lifecycle & main loop
├── spider.cpp         # Spider entity (legs, micro-animations)
├── state_machine.cpp  # AI behavior state machine
├── movement.cpp       # Steering behaviors (seek/flee/wander)
├── renderer.cpp       # SDL2 procedural rendering
├── tray.cpp           # Native system tray (Shell_NotifyIcon / X11 XEmbed)
├── platform_linux.cpp # X11 transparency, click-through, always-on-top
└── platform_windows.cpp # DWM per-pixel alpha, layered window, click-through
```

## How It Works

1. **Transparent Window**: A full-screen borderless window is created with input passthrough (X11 ShapeInput / WS_EX_TRANSPARENT)
2. **State Machine**: Evaluates cursor distance and transitions between behavioral states
3. **Steering**: Vector-based movement with seek/flee/wander forces, friction, and speed limits
4. **Procedural Legs**: 8 legs with alternating gait pattern, IK-style knee positioning, and step arcs
5. **Rendering**: Pure SDL2 draw calls — circles, ellipses, and thick lines compose the spider
6. **System Tray**: Native OS tray icon with right-click context menu for runtime control

### Platform Transparency

- **Windows**: DWM glass compositing via `DwmExtendFrameIntoClientArea` with per-pixel alpha from the SDL2 renderer backbuffer
- **Linux**: 32-bit ARGB X11 visual selected via `SDL_HINT_VIDEO_X11_WINDOW_VISUALID`, composited by picom/compton/xcompmgr

## License

MIT
