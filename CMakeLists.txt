cmake_minimum_required(VERSION 3.10)
project(minimalist-lockscreen)

# Set the C standard to C23
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

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

# Link libraries
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
)

# Apply flags based on the build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(minimalist-lockscreen PRIVATE -g -Wall -Wextra)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(minimalist-lockscreen PRIVATE -O3)
endif()

# Include-What-You-Use (IWYU) integration
find_program(iwyu_path NAMES include-what-you-use iwyu)
if(NOT iwyu_path)
    message(WARNING "Could not find the program include-what-you-use")
else()
    set_property(TARGET minimalist-lockscreen PROPERTY C_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()

# Print the current build information
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "CMake system name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMake system version: ${CMAKE_SYSTEM_VERSION}")
message(STATUS "CMake C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "CMake CXX compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
