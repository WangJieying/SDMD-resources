#ifndef SKEL_CUDA_HPP
#define SKEL_CUDA_HPP

FIELD<float>* computeCUDADT(FIELD<float> *im, bool foreground);
int initialize_skeletonization(int xM, int yM);
void deallocateCudaMem();

#endif