cmake_minimum_required(VERSION 3.2)

# Compiling
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project
#########################
project(ludo-assimp)

# Project Dependencies
#########################

# Assimp
add_subdirectory(lib/assimp)

# Source
#########################
set(SRC_FILES
    src/ludo/assimp/animation.cpp
    src/ludo/assimp/importing.cpp
    src/ludo/assimp/math.cpp
    src/ludo/assimp/meshes.cpp
    src/ludo/assimp/physics.cpp
    src/ludo/assimp/textures.cpp
    src/ludo/assimp/util.cpp)

# Target
#########################
add_library(ludo-assimp STATIC ${SRC_FILES})
target_include_directories(ludo-assimp PUBLIC src)

# Target Dependencies
#########################

# Assimp
target_include_directories(ludo-assimp PUBLIC lib/assimp/include)
target_link_libraries(ludo-assimp assimp)

# ludo
target_link_libraries(ludo-assimp ludo)
