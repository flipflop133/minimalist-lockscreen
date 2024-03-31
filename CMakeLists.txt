cmake_minimum_required(VERSION 3.10)
project(minimalist-lockscreen)

add_executable(minimalist-lockscreen main.c pam.c args.c display.c)

# Add include directories
target_include_directories(minimalist-lockscreen PRIVATE
    /usr/include
    /usr/include/freetype2/
)

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
)

# Set compiler flags for release build
target_compile_options(minimalist-lockscreen PRIVATE -O3)

# Set compiler flags for debug build
target_compile_options(minimalist-lockscreen PRIVATE -g -Wall -Wextra)