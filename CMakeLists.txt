cmake_minimum_required(VERSION 3.10)
project(minimalist-lockscreen)

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

# Add include directories
target_include_directories(minimalist-lockscreen PRIVATE
    /usr/include
    /usr/include/freetype2/
    /usr/include/X11/extensions/
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
    Xss
)

# Set compiler flags for release build
target_compile_options(minimalist-lockscreen PRIVATE -O3)

# Set compiler flags for debug build
target_compile_options(minimalist-lockscreen PRIVATE -g -Wall -Wextra)

# execute iwyu
find_program(iwyu_path NAMES include-what-you-use iwyu)
if(NOT iwyu_path)
  message(WARNING "Could not find the program include-what-you-use")
else()
  set_property(TARGET minimalist-lockscreen PROPERTY C_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()