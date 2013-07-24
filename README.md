![XYZRGB Dragon](https://raw.github.com/tunabrain/sparse-voxel-octrees/master/Header.png)

Sparse Voxel Octrees
=========

This project provides a multithreaded, CPU Sparse Voxel Octree implementation in C++, capable of raytracing large datasets in real-time, converting raw voxel files to octrees and converting mesh data (in form of PLY files) to voxel octrees.

The conversion routines are capable of handling datasets much larger than the working memory, allowing the creation and rendering of very large octrees (resolution 8192x8192x8192 and up).

This implementation closely follows the paper [Efficient Sparse Voxel Octrees](https://research.nvidia.com/publication/efficient-sparse-voxel-octrees) by Samuli Laine and Tero Karras.

The XYZRGB dragon belongs the the Stanford 3D Scanning Repository and is available from [their homepage](http://graphics.stanford.edu/data/3Dscanrep/) 

Compilation
===========

Compilation requires the SDL (Simple DirectMedia Layer) library. The provided makefile should work on Windows (tested with MinGW) and Linux.

Usage
=====

On startup, the program will load the sample octree and render it. Left mouse rotates the model, right mouse zooms. Escape quits the program.

Note that due to repository size considerations, the sample octree has poor resolution (256x256x256). You can generate larger octrees using the code, however. See <code>Main.cpp:initScene</code> for details. 

Code
====

<code>Main.cpp</code> controls application setup, thread spawning and basic rendering (should move this into a different file instead at some point).

<code>VoxelOctree.cpp</code> provides routines for octree raymarching as well as generating, saving and loading octrees. It uses <code>VoxelData.cpp</code>, which robustly handles fast access to non-square, non-power-of-two voxel data not completely loaded in memory.

The <code>VoxelData</code> class can also pull voxel data directly from <code>PlyLoader.cpp</code>, generating data from triangle meshes on demand, instead of from file, which vastly improves conversion performance due to elimination of file I/O. 