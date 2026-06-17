cmake_minimum_required(VERSION 3.16)
project(spider-pet VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform-specific settings
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS -D_USE_MATH_DEFINES -DNOMINMAX -DUNICODE -D_UNICODE)
    set(PLATFORM_SOURCES src/platform_windows.cpp)
    set(PLATFORM_LIBS user32 shell32 gdi32)

    # Find SDL2 via vcpkg/find_package on Windows
    find_package(SDL2 CONFIG REQUIRED)
    set(SDL2_LIBS SDL2::SDL2 SDL2::SDL2main)

    # Hide console window on Windows (MSVC)
    set(CMAKE_WIN32_EXECUTABLE ON)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
    set(PLATFORM_SOURCES src/platform_linux.cpp)

    # Find SDL2 via pkg-config on Linux
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    set(SDL2_LIBS ${SDL2_LIBRARIES})

    pkg_check_modules(X11 REQUIRED x11)
    pkg_check_modules(XFIXES REQUIRED xfixes)
    pkg_check_modules(XEXT REQUIRED xext)
    find_package(Threads REQUIRED)
    set(PLATFORM_LIBS
        ${X11_LIBRARIES}
        ${XFIXES_LIBRARIES}
        ${XEXT_LIBRARIES}
        Threads::Threads
    )
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/app.cpp
    src/spider.cpp
    src/state_machine.cpp
    src/movement.cpp
    src/renderer.cpp
    src/tray.cpp
    ${PLATFORM_SOURCES}
)

add_executable(spider-pet ${SOURCES})

target_include_directories(spider-pet PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

if(NOT WIN32)
    target_include_directories(spider-pet PRIVATE
        ${X11_INCLUDE_DIRS}
        ${XFIXES_INCLUDE_DIRS}
        ${XEXT_INCLUDE_DIRS}
    )
endif()

target_link_libraries(spider-pet PRIVATE
    ${SDL2_LIBS}
    ${PLATFORM_LIBS}
)

if(NOT WIN32)
    target_link_libraries(spider-pet PRIVATE m)
    target_compile_options(spider-pet PRIVATE
        ${SDL2_CFLAGS_OTHER}
        ${X11_CFLAGS_OTHER}
        ${XFIXES_CFLAGS_OTHER}
        ${XEXT_CFLAGS_OTHER}
        -O2
    )
else()
    target_compile_options(spider-pet PRIVATE /O2)
endif()

# Install
install(TARGETS spider-pet DESTINATION bin)
