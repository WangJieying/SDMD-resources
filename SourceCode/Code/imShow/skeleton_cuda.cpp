/* skeleton.cpp */

#include <math.h>
#include "fileio/fileio.hpp"
#include <cuda_runtime_api.h>
#include "CUDASkel2D/include/genrl.h"
#include "CUDASkel2D/include/field.h"
#include "CUDASkel2D/include/skelft.h"
#include "skeleton_cuda.hpp"

#define INDEX(i,j) (i)+fboSize*(j)
float* siteParam;

unsigned char* inputFBO = 0; //Input image, 0-padded to fboSize^2
short* outputFT;
//bool* foreground_mask;
int xm = 0, ym = 0, xM, yM, fboSize;
float length;

void allocateCudaMem(int size) {
    skelft2DInitialization(size);
    cudaMallocHost((void**)&outputFT, size * size * 2 * sizeof(short));
    //cudaMallocHost((void**)&foreground_mask, size * size * sizeof(bool));
    cudaMallocHost((void**)&siteParam, size * size * sizeof(float));
}

void deallocateCudaMem() {
    skelft2DDeinitialization();
    cudaFreeHost(outputFT);
    //cudaFreeHost(foreground_mask);
    cudaFreeHost(siteParam);
}

int initialize_skeletonization(int xM, int yM) {
    fboSize = skelft2DSize(xM, yM); //   Get size of the image that CUDA will actually use to process our nx x ny image
    allocateCudaMem(fboSize);
    return fboSize;
}

// TODO(maarten): dit moet beter kunnen, i.e. in een keer de DT uit cuda halen
void dt_to_field(FIELD<float>* f) {
    for (int i = 0; i < xM; ++i) {
        for (int j = 0; j < yM; ++j) {
            int id = INDEX(i, j);
            int ox = outputFT[2 * id];
            int oy = outputFT[2 * id + 1];
            float val = sqrt((i - ox) * (i - ox) + (j - oy) * (j - oy));
            //if (foreground_mask[INDEX(i, j)]) f->set(i, j, val);
            //else f->set(i, j, 0);
            f->set(i, j, val);
        }
    }
}


FIELD<float>* computeCUDADT(FIELD<float> *input, bool foreground) {
    //cout<<"here?"<<endl;
    //int sign = 0;
    memset(siteParam, 0, fboSize * fboSize * sizeof(float));
    //memset(foreground_mask, false, fboSize * fboSize * sizeof(bool));
    
    int nx = input->dimX();
    int ny = input->dimY();
    xm = ym = nx; xM = yM = 0;

    // Invert the image if necessary
    if (!foreground) {
        for (int i = 0; i < nx; ++i) {
            for (int j = 0; j < ny; ++j) {
                input->set(i, j, 255 - input->value(i, j));
            }
        }
    }
    for (int i = 0; i < nx; ++i) {
        for (int j = 0; j < ny; ++j) {
            if (!(*input)(i, j)) {
                //sign = 1;// if the first layer is all-1, then it never go into here, so xm will larger than xM, that's where we have a problem in skelft2DMakeBoundary() fucntion. So I added a sign here.
                //foreground_mask[INDEX(i, j)] = true;
                siteParam[INDEX(i, j)] = 1;
                xm = min(xm, i); ym = min(ym, j);
                xM = max(xM, i); yM = max(yM, j);
            }
        }
    }
    //PRINT(MSG_NORMAL, "here?\n");
   //wang
    xM = (xm==nx)? (nx - 1): nx;//must run before xm=...
    yM = (xm==nx)? (ny - 1): ny;
    xm = (xm==0)? 0 : (xm-1);
    ym = (ym==0)? 0 : (ym-1);
   
    //xM = nx - 1; yM = ny - 1;
    
    skelft2DFT(0, siteParam, 0, 0, fboSize, fboSize, fboSize);
    
    skelft2DDT(outputFT, 0, xm, ym, xM, yM);
    //if (sign)
    
        length = skelft2DMakeBoundary((unsigned char*)outputFT, xm, ym, xM, yM, siteParam, fboSize, 0, false);//wang. here's the problem
        if (!length) return NULL;
    
    skelft2DFillHoles((unsigned char*)outputFT, xm + 1, ym + 1, 1);
    
    skelft2DFT(outputFT, siteParam, xm, ym, xM, yM, fboSize);
   
    dt_to_field(input);
    
    return input;
}