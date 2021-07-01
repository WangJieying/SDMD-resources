//CUDA-based Skeletonization, Distance Transforms, Feature Transforms, Erosion and Dilation, and Flood Filling Toolkit
//
//(c) Alexandru Telea, Univ. of Groningen, 2011
//====================================================================================================================
#pragma once



// Given an image of size nx x ny that we want to process, CUDA may need to use a larger image (e.g. pow 2) internally
// to handle our image. This returns the size of this larger image. Since we get back data from CUDA at this size,
// we need to know about the size to allocate all our app-side buffers.
int skelft2DSize(int nx, int ny);


// Initialize CUDA and allocate memory
// textureSize is 2^k with k >= 6
void skelft2DInitialization(int textureSize);


// Deallocate all memory on GPU
void skelft2DDeinitialization();


// Set various FT computation params. Can be called before each FT computation
void skelft2DParams(int phase1Band, int phase2Band, int phase3Band);


// Compute 2D feature transform (or Voronoi diagram)
// siteParam: 2D texture site parameterization. 0 = non-site pixel; >0 = site parameter at current pixel.
// output:    2D texture FT. Gives coords (i,j) of closest site to each pixel.
//            If output==0, the FT is still computed and stored on CUDA, but not passed back to CPU.
// size:
void skelft2DFT(short* output, float* siteParam, short xm, short ym, short xM, short yM, int size);


// Compute thresholded skeleton of in-CUDA-memory FT.
// length:    max value of site parameter (needed for normalization)
// threshold: threshold for the skeleton importance, like in the AFMM algorithm
// output:    binary thresholded skeleton (0=background,1=skeleton)
void skelft2DSkeleton(float* output, bool* od, float length, float threshold,
                      short xm, short ym, short xM, short yM);

// Compute thresholded DT of in-CUDA-memory FT.
// threshold: upper value for DT
// output:    binary thresholded DT (1=larger than threshold,0=otherwise)
void skelft2DDT(short* output, float threshold, short xm, short ym, short xM, short yM);


// Make an arc-length parameterization of a binary shape, used for skeletonization input
// input:	  binary image whose boundary we parameterize
// dx,dy:
// param:     arc-length parameterization image
// size:
// return:	  the boundary length
float skelft2DMakeBoundary(unsigned char* input, int xm, int ym, int xM, int yM, float* param, int size, short iso = 1, bool thr_upper = true);


// Compute topology events (skeleton endpoints) for an in-CUDA-memory thresholded skeleton
// topo:	  binary image (1=skel endpoints,0=otherwise), optional. If not supplied, not returned
// npts:	  on input, this gives the max #points we will return; on output, set to the #points detected and returned
// points:    array of (x,y) pairs of the detected points
extern "C" void skelft2DTopology(unsigned char* topo, int* npts, short* points,
                                 short xm, short ym, short xM, short yM);


// Utility: save given image to pgm file
void skelft2DSave(short* outputFT, int dx, int dy, const char* f);


// Compute DT of in-CUDA-memory skeleton
void skel2DSkeletonDT(float* outputSkelDT, short xm, short ym, short xM, short yM);
void skel2DSkeletonFT(short* outputSkelFT, short xm, short ym, short xM, short yM);


// Fills all same-value 4-connected pixels from (seedx,seedy) with value 'fill_val' in the in-CUDA-memory thresholded DT
// outputFill: filled image
// <return>:   #iterations done for the fill, useful as a performance measure (lower=better)
int skelft2DFill(unsigned char* outputFill, short seedx, short seedy, short xm, short ym, short xM, short yM, unsigned char foreground);



int skelft2DFillHoles(unsigned char* outputFill, short x, short y, unsigned char fill_val);

