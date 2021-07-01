#ifndef SKEL_CUDA_HPP
#define SKEL_CUDA_HPP

FIELD<float>* computeSkeleton(FIELD<float> *im, bool print);
int initialize_skeletonization(FIELD<float>* im);
void deallocateCudaMem();
short* get_current_skel_ft();
FIELD<float>* skelft_to_field();

#endif