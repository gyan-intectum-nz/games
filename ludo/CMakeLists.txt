cmake_minimum_required(VERSION 3.2)

# Compiling
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project
#########################
project(ludo)

# Source
#########################
set(SRC_FILES
    src/ludo/animation.cpp
    src/ludo/core.cpp
    src/ludo/data/buffers.cpp
    src/ludo/data/data.cpp
    src/ludo/data/heaps.cpp
    src/ludo/files.cpp
    src/ludo/math/distance.cpp
    src/ludo/math/mat.cpp
    src/ludo/math/projection.cpp
    src/ludo/math/quat.cpp
    src/ludo/math/util.cpp
    src/ludo/math/vec.cpp
    src/ludo/meshes.cpp
    src/ludo/meshes/box.cpp
    src/ludo/meshes/circle.cpp
    src/ludo/meshes/clean.cpp
    src/ludo/meshes/collapse.cpp
    src/ludo/meshes/cylinder.cpp
    src/ludo/meshes/edit.cpp
    src/ludo/meshes/math.cpp
    src/ludo/meshes/rectangle.cpp
    src/ludo/meshes/sphere_cube.cpp
    src/ludo/meshes/sphere_ico.cpp
    src/ludo/meshes/sphere_uv.cpp
    src/ludo/meshes/util.cpp
    src/ludo/rendering.cpp
    src/ludo/spatial/bounds.cpp
    src/ludo/spatial/grid2.cpp
    src/ludo/spatial/grid3.cpp
    src/ludo/spatial/octree.cpp
    src/ludo/spatial/quadtree.cpp
    src/ludo/testing.cpp
    src/ludo/thread_pool.cpp
    src/ludo/timer.cpp)

set(TEST_SRC_FILES
    tests/data/arrays.cpp
    tests/data/buffers.cpp
    tests/math/mat.cpp
    tests/math/projection.cpp
    tests/math/quat.cpp
    tests/math/vec.cpp
    tests/spatial/grid2.cpp
    tests/spatial/grid3.cpp
    tests/spatial/octree.cpp
    tests/spatial/quadtree.cpp
    tests/tests.cpp)

# Target
#########################
add_library(ludo STATIC ${SRC_FILES})
target_include_directories(ludo PUBLIC src)

# Test Target
#########################
add_executable(ludo-tests ${SRC_FILES} ${TEST_SRC_FILES})
target_include_directories(ludo-tests PUBLIC src tests)
