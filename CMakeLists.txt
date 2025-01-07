cmake_minimum_required(VERSION 3.21)
project(minimalist-lockscreen C)

# Include common settings
include(build-scripts/common.cmake)

add_compile_definitions(_GNU_SOURCE)

# Add source files
add_executable(minimalist-lockscreen
    src/main.c
    src/lockscreen.c
    src/utils.c
    src/pam.c
    src/args.c
    src/graphics/graphics.c
    src/graphics/modules/date.c
    src/graphics/modules/password_entry.c
)

# Add include directories specific to this project
target_include_directories(minimalist-lockscreen PRIVATE
    /usr/include/X11/extensions/
)

# Add libraries specific to this project
target_link_libraries(minimalist-lockscreen PRIVATE
    X11
    Xft
    Xrandr
    Xinerama
    ssl
    crypto
    pam
    cairo
    Xfixes
    Xss
    Xext
    m
    fontconfig
)
