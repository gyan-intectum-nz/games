cmake_minimum_required(VERSION 3.2)

# Compiling
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project
#########################
project(astrum)

# Project Dependencies
#########################

# ludo
add_subdirectory(../ludo lib/ludo)
add_subdirectory(../ludo-assimp lib/ludo-assimp)
add_subdirectory(../ludo-bullet lib/ludo-bullet)
add_subdirectory(../ludo-glfw lib/ludo-glfw)
add_subdirectory(../ludo-opengl lib/ludo-opengl)
add_subdirectory(../ludo-stb lib/ludo-stb)

# noise
add_subdirectory(lib/libnoise)

# Source
#########################
set(SRC_FILES
    src/controllers/game.cpp
    src/controllers/map.cpp
    src/controllers/person.cpp
    src/controllers/spaceship.cpp
    src/entities/luna.cpp
    src/entities/people.cpp
    src/entities/sol.cpp
    src/entities/spaceships.cpp
    src/entities/terra.cpp
    src/entities/trees.cpp
    src/main.cpp
    src/meshes/ico_chunks.cpp
    src/meshes/ico_faces.cpp
    src/meshes/lod_shaders.cpp
    src/meshes/lods.cpp
    src/paths.cpp
    src/physics/centering.cpp
    src/physics/gravity.cpp
    src/physics/point_masses.cpp
    src/physics/relativity.cpp
    src/physics/util.cpp
    src/post-processing/atmosphere.cpp
    src/post-processing/bloom.cpp
    src/post-processing/pass.cpp
    src/post-processing/tone_mapping.cpp
    src/post-processing/util.cpp
    src/solar_system.cpp
    src/spatial/icotree.cpp
    src/terrain/mesh.cpp
    src/terrain/metadata.cpp
    src/terrain/static_bodies.cpp
    src/terrain/terrain.cpp
    src/terrain/terrain_chunk.cpp
    src/terrain/mesh.cpp
    src/util.cpp)

# Target
#########################
add_executable(astrum ${SRC_FILES})
target_include_directories(astrum PUBLIC src)

# Target Dependencies
#########################

# ludo
target_link_libraries(astrum ludo)
target_link_libraries(astrum ludo-assimp)
target_link_libraries(astrum ludo-bullet)
target_link_libraries(astrum ludo-glfw)
target_link_libraries(astrum ludo-opengl)
target_link_libraries(astrum ludo-stb)

# noise
target_include_directories(astrum PUBLIC lib/libnoise/src)
target_link_libraries(astrum noise)

# pthread
IF(UNIX)
    target_link_libraries(astrum pthread)
ENDIF(UNIX)

# Target Resources
#########################
add_custom_command(TARGET astrum PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets ${PROJECT_BINARY_DIR}/assets)

# Demo Targets
#########################
add_executable(loddy src/demos/loddy.cpp src/meshes/lod_shaders.cpp src/meshes/lods.cpp)

# Demo Target Dependencies
#########################

# ludo
target_link_libraries(loddy ludo)
target_link_libraries(loddy ludo-assimp)
target_link_libraries(loddy ludo-bullet) # TODO revise, only for assimp...
target_link_libraries(loddy ludo-glfw)
target_link_libraries(loddy ludo-opengl)
target_link_libraries(loddy ludo-stb)

# Demo Target Resources
#########################
add_custom_command(TARGET loddy PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets ${PROJECT_BINARY_DIR}/assets)
