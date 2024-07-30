cmake_minimum_required(VERSION 3.2)

# Compiling
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project
#########################
project(ludo-bullet)

# Project Dependencies
#########################

# Bullet3
set(BUILD_EXTRAS ON)
set(BUILD_SERIALIZE_EXTRA ON)
add_subdirectory(lib/bullet3)

# Source
#########################
set(SRC_FILES
    src/ludo/bullet/debug.cpp
    src/ludo/bullet/math.cpp
    src/ludo/bullet/physics.cpp)

set(BULLET_SRC_FILES
    lib/bullet3/Extras/Serialize/BulletFileLoader/bChunk.cpp
    lib/bullet3/Extras/Serialize/BulletFileLoader/bDNA.cpp
    lib/bullet3/Extras/Serialize/BulletFileLoader/bFile.cpp
    lib/bullet3/Extras/Serialize/BulletFileLoader/btBulletFile.cpp
    lib/bullet3/Extras/Serialize/BulletWorldImporter/btBulletWorldImporter.cpp
    lib/bullet3/Extras/Serialize/BulletWorldImporter/btMultiBodyWorldImporter.cpp
    lib/bullet3/Extras/Serialize/BulletWorldImporter/btWorldImporter.cpp)

# Target
#########################
add_library(ludo-bullet STATIC ${SRC_FILES} ${BULLET_SRC_FILES})
target_include_directories(ludo-bullet PUBLIC src)

# Target Dependencies
#########################

# Bullet3
target_include_directories(ludo-bullet PUBLIC lib/bullet3/src lib/bullet3/Extras/Serialize/BulletWorldImporter)
target_link_libraries(ludo-bullet BulletCollision BulletDynamics LinearMath)

# ludo
target_link_libraries(ludo-bullet ludo)
