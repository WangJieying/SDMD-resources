#include "include/skelft.h"
#include "include/field.h"
#include "include/vis.h"
#include <cuda_runtime_api.h>

#include <math.h>
#include <iostream>
#include <time.h>

using namespace std;




// Initialization
void initialization(float*& siteParam, short*& outputFT, unsigned char*& outputSkeleton, short*& outputTopo, float*& skelDT, int size) {
    skelft2DInitialization(size);

    cudaMallocHost((void**)&outputFT, size * size * 2 * sizeof(short));
    cudaMallocHost((void**)&outputSkeleton, size * size * sizeof(unsigned char));
    cudaMallocHost((void**)&siteParam, size * size * sizeof(float));
    cudaMallocHost((void**)&outputTopo, size * size * 2 * sizeof(short));
    cudaMallocHost((void**)&skelDT, size * size * 2 * sizeof(float));
}



// Deinitialization
void deinitialization(float* siteParam, short* outputFT, unsigned char* outputSkeleton, short* outputTopo, float* skelDT) {
    skelft2DDeinitialization();

    cudaFreeHost(outputFT);
    cudaFreeHost(outputSkeleton);
    cudaFreeHost(siteParam);
    cudaFreeHost(outputTopo);
    cudaFreeHost(skelDT);
}






int main(int argc, char **argv) {
    cout << "CUDA DT/FT/Skeletonization" << endl << endl;
    cout << "  +,-:     skeleton simplification level" << endl;
    cout << "  <,>:     zoom in/out" << endl;
    cout << "    t:     texture interpolation on/off" << endl;
    cout << "  space:   display skeleton / FT" << endl;
    if (argc < 2) return 1;

    float* siteParam;													//Boundary parameterization: fboSize x fboSize. value(i,j) = boundary-param of (i,j) if on boundary, else unused
    short* outputFT;													//1-point FT output: fboSize x fboSize x 2. value(i,j) = the two coords of closest site to (i,j)
    unsigned char* outputSkeleton;										//Skeleton: fboSize x fboSize. value(i,j) = skeleton at (i,j) (i.e. zero or one)
    short* outputTopo;
    float* skelDT;
    short xm, ym, xM, yM;


    FIELD<int>* input = FIELD<int>::read(argv[1]);						//1. Read scalar field input
    int nx = input->dimX();
    int ny = input->dimY();
    int fboSize = skelft2DSize(nx, ny);									//   Get size of the image that CUDA will actually use to process our nx x ny image
    initialization(siteParam, outputFT, outputSkeleton, outputTopo, skelDT, fboSize);	//Allocate enough space for the max image size
    //REMARK: we can allocate stricter e.g. use 'fboSize', but then we may need to reallocate
    //if we process several differently-sized images in a sequence. This is faster.
    memset(siteParam, 0, fboSize * fboSize * sizeof(float));					//2. Create siteParam simply by thresholding input image (we'll only use it for DT)

    xm = ym = nx; xM = yM = 0;
    for (int i = 0; i < nx; ++i)
        for (int j = 0; j < ny; ++j)
            if (!(*input)(i, j)) {
                siteParam[j * fboSize + i] = 1;
                xm = min(xm, i); ym = min(ym, j);
                xM = max(xM, i); yM = max(yM, j);
            }
    xM = nx - 1; yM = ny - 1;

    delete input;

    int nendpoints = 2000;
    float threshold = 20;												//Skeleton lower threshold
    int inflation_dist = 0;

    clock_t ts = clock();												//Start timing

    skelft2DFT(0, siteParam, 0, 0, fboSize, fboSize, fboSize);				//3. Compute FT of the sites (for inflation)
    skelft2DDT(outputFT, inflation_dist, xm, ym, xM, yM);					//4. Inflate input shape with some distance (by thresholding its DT). Reuse outputFT to save thresholded-DT

    clock_t t0 = clock();

    xm -= inflation_dist; ym -= inflation_dist;							//Adjust bounds with inflation_dist; careful not to exceed img size (not done!!)
    xM += inflation_dist; yM += inflation_dist;

    //skelft2DFillHoles((unsigned char*)outputFT,xm+1,ym+1,0,1);		//Slow..
    int iter = skelft2DFill((unsigned char*)outputFT, xm + 1, ym + 1, xm - 2, ym - 2, xM + 2, yM + 2, 128); //Fill background in shape with value 128 (i.e. sth which isn't foreground or background)

    clock_t t1 = clock();

    float length = skelft2DMakeBoundary((unsigned char*)outputFT, xm, ym, xM, yM, siteParam, fboSize, 127, false);
    clock_t t2 = clock();
    //5. Parameterize boundary of inflated shape into 'siteParam' for skeleton computation

    skelft2DFT(outputFT, siteParam, xm, ym, xM, yM, fboSize);			//6. Compute FT of 'siteParam'
    clock_t t3 = clock();
    skelft2DSkeleton(outputSkeleton, length, threshold, xm, ym, xM, yM);	//7. Skeletonize the FT into 'outputSkeleton'
    clock_t t4 = clock();
    skelft2DTopology(0, &nendpoints, outputTopo, xm, ym, xM, yM);				//8. Detect endpoints of the skeleton, put them in outputTopo[]
    clock_t t5 = clock();

    skel2DSkeletonDT(skelDT, xm, ym, xM, yM);

    float time  = float(t5 - ts) / CLOCKS_PER_SEC;
    float tto   = float(t5 - t4) / CLOCKS_PER_SEC;
    float tsk   = float(t4 - t3) / CLOCKS_PER_SEC;
    float tft   = float(t3 - t2) / CLOCKS_PER_SEC;
    float tbo   = float(t2 - t1) / CLOCKS_PER_SEC;
    float tfi   = float(t1 - t0) / CLOCKS_PER_SEC;


    cout << "-----------------" << endl;
    cout << "Texture: " << fboSize << "x" << fboSize << endl;
    cout << "Time: " << time << " secs (all) " << tft << " (FT) " << tsk << " (skel) " << tto << " (topo) " << tbo << " (boundary) " << tfi << " (fill, iters=" << iter << ")" << endl;
    cout << "-----------------" << endl;

    Display* dpy = new Display(800, fboSize, outputFT, outputSkeleton, siteParam, nendpoints, outputTopo, skelDT, length, threshold, xm, ym, xM, yM, argc, argv);
    //Initialize visualization engine
    dpy->generateTexture();												//Show results
    glutMainLoop();

    deinitialization(siteParam, outputFT, outputSkeleton, outputTopo, skelDT);
    delete dpy;
    return 0;
}



