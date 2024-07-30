cmake_minimum_required(VERSION 3.2)

# Compiling
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project
#########################
project(ludo-glfw)

# Project Dependencies
#########################

# GLFW
add_subdirectory(lib/glfw)

# Source
#########################
set(SRC_FILES
    src/ludo/glfw/input.cpp
    src/ludo/glfw/windowing.cpp)

# Target
#########################
add_library(ludo-glfw STATIC ${SRC_FILES})
target_include_directories(ludo-glfw PUBLIC src)

# Target Dependencies
#########################

# GLFW
target_include_directories(ludo-glfw PUBLIC lib/glfw/include)
target_link_libraries(ludo-glfw glfw ${GLFW_LIBRARIES})

# ludo
target_link_libraries(ludo-glfw ludo)
