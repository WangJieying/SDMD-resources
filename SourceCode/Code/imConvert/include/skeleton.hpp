/* skeleton.hpp */
#ifndef SKELETON_GUARD
#define SKELETON_GUARD

#include "fileio/fileio.hpp"
#include "afmmstarvdt/include/mfmm.h"
#include "afmmstarvdt/include/skeldt.h"
#include "afmmstarvdt/include/genrl.h"
#include "afmmstarvdt/include/field.h"
#include "afmmstarvdt/include/flags.h"
FIELD<float>*       computeSkeleton     (FIELD<float> *im, int intensity);
FIELD<float>*       do_one_pass         (FIELD<float>* fi, FLAGS* flagsi, FIELD<std::multimap<float, int> >* origs, int dir, ModifiedFastMarchingMethod::METHOD meth);
void                postprocess         (FIELD<float>* grad, FIELD<float>* skel, float level);
void                comp_grad2          (FIELD<float>* cnt1, FIELD<float>* cnt2, FIELD<float>* grad);
FIELD<float>*       saliencyThreshold   (FIELD<float>* distMap, FIELD<float> *grad);
void initialize_skeletonization(FIELD<float>* im) { /* noting */}
#endif
