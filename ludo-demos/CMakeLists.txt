cmake_minimum_required(VERSION 3.2)

# Compiling
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project
#########################
project(ludo-demos)

# Project Dependencies
#########################

# ludo
add_subdirectory(../ludo lib/ludo)
add_subdirectory(../ludo-assimp lib/ludo-assimp)
add_subdirectory(../ludo-bullet lib/ludo-bullet)
add_subdirectory(../ludo-glfw lib/ludo-glfw)
add_subdirectory(../ludo-opengl lib/ludo-opengl)
add_subdirectory(../ludo-stb lib/ludo-stb)

# Targets
#########################
add_executable(importy src/importy.cpp)
add_executable(noisy src/noisy.cpp)
add_executable(physicy src/physicy.cpp)
add_executable(spinny src/spinny.cpp)

# Target Dependencies
#########################

# ludo
target_link_libraries(importy ludo)
target_link_libraries(importy ludo-assimp)
target_link_libraries(importy ludo-bullet) # TODO revise, only for assimp...
target_link_libraries(importy ludo-glfw)
target_link_libraries(importy ludo-opengl)
target_link_libraries(importy ludo-stb)
target_link_libraries(noisy ludo)
target_link_libraries(noisy ludo-glfw)
target_link_libraries(noisy ludo-opengl)
target_link_libraries(physicy ludo)
target_link_libraries(physicy ludo-bullet)
target_link_libraries(physicy ludo-glfw)
target_link_libraries(physicy ludo-opengl)
target_link_libraries(spinny ludo)
target_link_libraries(spinny ludo-glfw)
target_link_libraries(spinny ludo-opengl)
target_link_libraries(spinny ludo-stb)

# Target Resources
#########################
add_custom_command(TARGET importy PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets ${PROJECT_BINARY_DIR}/assets)
add_custom_command(TARGET spinny PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets ${PROJECT_BINARY_DIR}/assets)
