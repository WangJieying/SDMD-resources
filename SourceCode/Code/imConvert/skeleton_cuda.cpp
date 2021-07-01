/* skeleton.cpp */

#include <math.h>
#include "fileio/fileio.hpp"
#include <cuda_runtime_api.h>
#include "CUDASkel2D/include/genrl.h"
#include "CUDASkel2D/include/field.h"
#include "CUDASkel2D/include/skelft.h"
#include "ImageWriter.hpp"
#include "skeleton_cuda.hpp"
#include "include/connected.hpp"
#include "include/messages.h"


float SKELETON_SALIENCY_THRESHOLD;
//float SKELETON_ISLAND_THRESHOLD;
float        SKELETON_DT_THRESHOLD;

#define INDEX(i,j) (i)+fboSize*(j)

float* siteParam;

// The following are CUDA buffers for several images.
//unsigned char* outputSkeleton;
float* outputSkeleton;
short* outputFT;
short* skelFT;
bool* foreground_mask;
int xm = 0, ym = 0, xM, yM, fboSize;
double length;

void allocateCudaMem(int size) {
    skelft2DInitialization(size);
    cudaMallocHost((void**)&outputFT, size * size * 2 * sizeof(short));
    cudaMallocHost((void**)&foreground_mask, size * size * sizeof(bool));
    cudaMallocHost((void**)&outputSkeleton, size * size * sizeof(float));
    cudaMallocHost((void**)&siteParam, size * size * sizeof(float));
    cudaMallocHost((void**)&skelFT, size * size * 2 * sizeof(short));
}

void deallocateCudaMem() {
    skelft2DDeinitialization();
    cudaFreeHost(outputFT);
    cudaFreeHost(foreground_mask);
    cudaFreeHost(outputSkeleton);
    cudaFreeHost(siteParam);
    cudaFreeHost(skelFT);
}


int initialize_skeletonization(FIELD<float>* im) {
    xM = im->dimX();
    yM = im->dimY();
    fboSize = skelft2DSize(xM + yM, yM + xM);//   Get size of the image that CUDA will actually use to process our nx x ny image
    //fboSize = 4096;
    allocateCudaMem(fboSize);
    return fboSize;
}


FIELD<float>* skel_to_field(int dimX, int dimY) {
    FIELD<float>* f = new FIELD<float>(dimX, dimY);
    for (int i = 0; i < dimX; ++i) {
        for (int j = 0; j < dimY; ++j) {
            //bool is_skel_point = outputSkeleton[INDEX(i, j)];
            //f->set(i, j, is_skel_point ? 255 : 0);
            f -> set(i,j,outputSkeleton[INDEX(i, j)]);
        }
    }
    return f;
}


// TODO(maarten): dit moet beter kunnen, i.e. in een keer de DT uit cuda halen
void dt_to_field(FIELD<float>* f) {
    for (int i = 0; i < xM; ++i) {
        for (int j = 0; j < yM; ++j) {
            int id = INDEX(i, j);
            int ox = outputFT[2 * id];
            int oy = outputFT[2 * id + 1];
            float val = sqrt((i - ox) * (i - ox) + (j - oy) * (j - oy));
            f->set(i, j, val);
        }
    }
}

FIELD<float>* skelft_to_field() {
 
    FIELD<float>* f = new FIELD<float>(xM, yM);
    
    for (int i = 0; i < xM; ++i) {
        for (int j = 0; j < yM; ++j) {
            int id = INDEX(i, j);
            int ox = skelFT[2 * id];
           // PRINT(MSG_NORMAL, "f2: %d\n", ox);
            int oy = skelFT[2 * id + 1];
            double val = 255 * (siteParam[oy * fboSize + ox] / length);
            f->set(i, j, val);
        }
    }
    return f;
}

FIELD<float>* computeSkeleton(FIELD<float> *input, bool print) {

    memset(siteParam, 0, fboSize * fboSize * sizeof(float));
   
    memset(foreground_mask, false, fboSize * fboSize * sizeof(bool));
    
    //unsigned char* image = new unsigned char[fboSize*fboSize];

    int nx = input->dimX();
    int ny = input->dimY();
    xm = ym = nx; 
    for (int i = 0; i < nx; ++i) {
        for (int j = 0; j < ny; ++j) {
            if (!(*input)(i, j)) {//0
                foreground_mask[INDEX(i, j)] = true;
                siteParam[INDEX(i, j)] = 1;
                //image[j*fboSize+i] = 255;
                xm = min(xm, i); ym = min(ym, j);
            }
        }
    }
    xM = nx; yM = ny;

    skelft2DFT(0, siteParam, 0, 0, fboSize, fboSize, fboSize);
    skelft2DDT(outputFT, 0, xm, ym, xM, yM);
    //length = skelft2DMakeBoundary((unsigned char*)outputFT, xm, ym, xM, yM, siteParam, fboSize, 0, false);
	length = skelft2DMakeBoundary((unsigned char*)outputFT, xm, ym, xM, yM, siteParam, fboSize, 1, true);

    if (!length) return NULL;
    skelft2DFillHoles((unsigned char*)outputFT, xm + 1, ym + 1, 1);
    skelft2DFT(outputFT, siteParam, xm, ym, xM, yM, fboSize);
    //cout<<"SKELETON_SALIENCY_THRESHOLD: "<<SKELETON_SALIENCY_THRESHOLD<<endl;
    skelft2DSkeleton(outputSkeleton, foreground_mask, length, SKELETON_SALIENCY_THRESHOLD, xm, ym, xM, yM);
 
    skel2DSkeletonFT(skelFT, xm, ym, xM, yM);
    dt_to_field(input);
    auto imp = skel_to_field(nx, ny);
    //auto skel = imp_to_skel(imp);
   //if (level==53) skel->writePGM("skel53.pgm");
    //analyze_cca(level, skel);
    return imp;
}


short* get_current_skel_ft() {
    size_t sz = fboSize * fboSize * 2;
    short* skelft = (short*)(malloc(sz * sizeof(short)));
    if (!skelft) {
        PRINT(MSG_ERROR, "Error: could not allocate array for skeleton FT\n");
        exit(-1);
    }
    memcpy(skelft, skelFT, sz);
    return skelft;
}
