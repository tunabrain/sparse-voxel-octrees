![XYZRGB Dragon](https://raw.github.com/tunabrain/sparse-voxel-octrees/master/Header.png)

Sparse Voxel Octrees
=========

This project provides a multithreaded, CPU Sparse Voxel Octree implementation in C++, capable of raytracing large datasets in real-time, converting raw voxel files to octrees and converting mesh data (in form of PLY files) to voxel octrees.

The conversion routines are capable of handling datasets much larger than the working memory, allowing the creation and rendering of very large octrees (resolution 8192x8192x8192 and up).

This implementation closely follows the paper [Efficient Sparse Voxel Octrees](https://research.nvidia.com/publication/efficient-sparse-voxel-octrees) by Samuli Laine and Tero Karras.

The XYZRGB dragon belongs the the Stanford 3D Scanning Repository and is available from [their homepage](http://graphics.stanford.edu/data/3Dscanrep/) 

Compilation
===========

A recent compiler cupporting C++11, CMake 2.8 and SDL 1.2 are required to build.

To build on Linux, you can use the `setup_builds.sh` shell script to setup build and release configurations using CMake. After running `setup_builds.sh`, run `make` inside the newly created `build/release/` folder. Alternatively, you can use the standard CMake CLI to configure the project.

To build on Windows, you will need Visual Studio 2013 or later. Before running CMake, make sure that

* CMake is on the `PATH` environment variable. An easy check to verify that this is the case is to open CMD and type `cmake`, which should output the CMake CLI help.
* You have Windows 64bit binaries of SDL 1.2. These are available [here](https://www.libsdl.org/download-1.2.php). Make sure to grab the `SDL-devel-1.2.XX-VC.zip`. Note that if you are using a newer version of Visual Studio, you may need to compile SDL yourself in order to be compatible. Please see the SDL website for details
* The environment variable `SDLDIR` exists and is set to the path to the folder containing SDL1.2 (you will have to set it up manually - it needs to be a system environment variable, not a user variable, for CMake to find it). CMake will use this variable to find the SDL relevant files and configure MSVC to use them

After these prerequisites are setup, you can run `setup_builds.bat` to create the Visual Studio files. It will create a folder `vstudio` containing the `sparse-voxel-octrees.sln` solution.

Alternatively, you can also run CMake manually or setup the MSVC project yourself, without CMake. The sources don't require special build flags, so the latter is easily doable if you can't get CMake to work.

To build on macOS, you will need to install SDL first (i.e. `brew install sdl`). Then build it like a regular CMake project:

    mkdir build
    cd build
    cmake ../
    make
    ./sparse-voxel-octrees -viewer ../models/XYZRGB-Dragon.oct

On macOS, you may need to click+drag within the application window first to make the render visible.

Note: If building fails on macOS, you can try commenting out the follow lines in <code>CMakeLists.txt</code>

    #if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    #    set(Sources ${Sources} "src/SDLMain.m")
    #endif()

Usage
=====

On startup, the program will load the sample octree and render it. Left mouse rotates the model, right mouse zooms. Escape quits the program. In order to make CLI arguments easier on Windows, you can use <code>run_viewer.bat</code> to start the viewer.

Note that due to repository size considerations, the sample octree has poor resolution (256x256x256). You can generate larger octrees using the code, however. See <code>Main.cpp:initScene</code> for details. You can also use <code>run_builder.bat</code> to build the XYZ RGB dragon model. To do this, simply download the XYZ RGB dragon model from http://graphics.stanford.edu/data/3Dscanrep/ and place it in the <code>models</code> folder.

Code
====

<code>Main.cpp</code> controls application setup, thread spawning and basic rendering (should move this into a different file instead at some point).

<code>VoxelOctree.cpp</code> provides routines for octree raymarching as well as generating, saving and loading octrees. It uses <code>VoxelData.cpp</code>, which robustly handles fast access to non-square, non-power-of-two voxel data not completely loaded in memory.

The <code>VoxelData</code> class can also pull voxel data directly from <code>PlyLoader.cpp</code>, generating data from triangle meshes on demand, instead of from file, which vastly improves conversion performance due to elimination of file I/O. 
