# Games Monorepo

This is a monorepo of all of Intectum's internal game projects. It includes an in-house game engine called ludo.

## Quick Start Guide

### Clone

```
git clone https://github.com/gyan-intectum-nz/games.git --recurse-submodules
```

### Install Dependencies (Ubuntu)

```
sudo apt install freeglut3-dev xorg-dev
```

### Build and Run Astrum

#### Using CLion

Open the `games` folder with CLion and make sure the `astrum/CMakeLists.txt` project is loaded. Run `astrum`.

#### Using CMake/Make

```
# cmake
mkdir games/astrum/build
cd games/astrum/build
cmake ..

# make
make astrum -j14

# run
./astrum
```

## Projects

- astrum - A tech demo based on some of Sebastian Lague's [Coding Adventures](https://www.youtube.com/playlist?list=PLFt_AvWsXl0ehjAfLFsp1PGaatzAwo0uK) videos
- ludo - A simple, procedural game engine
- ludo-assimp - An [Assimp](https://assimp.org/) plugin for ludo
- ludo-bullet - A [Bullet Physics](https://pybullet.org) plugin for ludo
- ludo-demos - A set of very basic demos used to verify the functionality of ludo
- ludo-glfw - A [GLFW](https://www.glfw.org/) plugin for ludo
- ludo-opengl - An [OpenGL](https://www.opengl.org/) plugin for ludo
