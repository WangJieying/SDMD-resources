CUDA-based Skeletonization, Distance Transforms, Feature Transforms, Erosion and Dilation, and Flood Filling Toolkit

(c) Alexandru Telea, Univ. of Groningen, 2011
====================================================================================================================

This software package implements 2D skeletonization, distance transforms, one-point feature transforms, erosion, dilation, and flood filling using CUDA.
All operations work on grayscale images in terms of input and output.

1. Building
===========

make

The software needs a C++ compiler, OpenGL, the CUDA 1.1 SDK, and GLUT to build. It may be necessary to modify the makefile and/or do some small-scale #include adaptions to cope
with variations on where the GL, GLUT, and CUDA headers are. The build was tested on Mac OS X and Linux with gcc 4.2. It should be trivial to adapt for e.g. Windows/Visual Studio.

2. Running
==========

skeleton somefile.pgm

where the input is a binary 8-bit-per-pixel grayscale PGM image. Typical such images are black-white drawings, where black=foreground, white=background.
Image examples are in the DATA directory.

3. API
======

See main.cpp for a very simple program that computes all above-mentioned quantities (skeletons, DT, FT, erosion/dilation).

The API is in include/skelft.h. There are several functions which work as follows (see skelft.h for latest updated information).


skelft2DInitialization
Input: none
Output: none

Initializes memory buffers on the CPU and GPU side. One parameter is given: the size of the largest image ever to be processed (must be power of 2).
This helps in doing memory allocation/deallocation only once per program run.


skelft2DDeinitialization
Input: none
Output :none

Frees all CPU/GPU memory buffers, to be done last.


skelft2DFT
Input: site parameterization: an image where each pixel has the value of 0 if it isn't a site or >0 if it is a site. Sites are the points that the DT/FT is computed from.
Output: FT: an image where each pixel has two integer coords of the site closest to it. The FT is kept stored on the GPU and only passed to the CPU if so desired.

Computes the FT of an input image where sites are marked.


skelft2DDT
Input: none (uses the in-CUDA FT which needs to be computed)
Output: binary thresholding of the DT corresponding to the stored FT, for a user-given threshold

Computes the DT for the in-CUDA stored FT, and binary-thresholds it.


skelft2DSkeleton
Input: none
Output: thresholded skeleton of the in-CUDA FT

Computes the skeleton for the in-CUDA stored FT, upper thresholds it for a user-given value, and returns the binary skeleton result.
For this to work, an FT must be computed from a site image where sites are encoded using a boundary-arc-length parameterization, as in the original AFMM algorithm.


4. Other remarks
================

Note that for maximal performance one should minimize data transfer from CPU to GPU memory. This means only passing those datasets to the GPU that one actually needs as inputs for
the desired image processing operations, and only retrieving the desired results. Concretely, for an image processing pipeline with several steps, it is advisable NOT to transfer
back from the GPU the intermediate results, if these are not needed explicitly on the CPU, since GPU-to-CPU transfer is costly.




