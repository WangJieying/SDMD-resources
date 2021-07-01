/* skeleton.cpp */

#include <math.h>
#include "fileio/fileio.hpp"
#include "afmmstarvdt/include/mfmm.h"
#include "afmmstarvdt/include/skeldt.h"
#include "afmmstarvdt/include/genrl.h"
#include "afmmstarvdt/include/flags.h"
#include "ImageWriter.hpp"
#include "skeleton.hpp"
#include "include/connected.hpp"
#include "include/messages.h"
#include "ImageStatistics.hpp"
#include "../shared/afmmstarvdt/include/safemalloc.hpp"

float SKELETON_SALIENCY_THRESHOLD;
float SKELETON_ISLAND_THRESHOLD;
float        SKELETON_DT_THRESHOLD;


#define EPSILON 0.00000001

float skeletonThreshold = 15;

FIELD<float>* computeSkeleton(FIELD<float> *im, int intensity) {
    FLAGS *flags;
    FIELD<float> *cnt0,
          *cnt1,
          *grad;
    FIELD<float> *skel;
    FIELD<std::multimap<float, int> >* origs = 0;

    /* Default to AFMM-Star, see afmm/README.txt */
    ModifiedFastMarchingMethod::METHOD meth = ModifiedFastMarchingMethod::AFMM_STAR;
    grad = new FIELD<float>(im->dimX(), im->dimY());
    skel = new FIELD<float>(im->dimX(), im->dimY());

    flags = new FLAGS(*im, -1);

    /* Two passes for boundary treatment */
    cnt0 = do_one_pass(im, flags, origs, 0, meth);
    cnt1 = do_one_pass(im, flags, origs, 1, meth);

    comp_grad2(cnt0, cnt1, grad);

    /* Compute skeleton using distance transform */

    postprocess(grad, skel, SKELETON_DT_THRESHOLD);
    IS_analyseLayer("DT", skel, intensity);

    /* Compute skeleton using saliency */
    FIELD<float> *ss = saliencyThreshold(im, grad);
    IS_analyseLayer("saliency", ss, intensity);
    /* intersect previous two skeletons */
    FIELD<float> *dblFiltered = new FIELD<float>(ss->dimX(), ss->dimY());
    for (int y = 0; y < ss->dimY(); ++y) {
        for (int x = 0; x < ss->dimX(); ++x) {
            dblFiltered->set(x, y, (ss->value(x, y) != 0 && skel->value(x, y) != 0) * 255.0);
        }
    }
    IS_analyseLayer("DTsaliency", dblFiltered, intensity);

    delete cnt0;
    delete cnt1;
    delete grad;
    delete flags;
    delete origs;
    delete ss;
    delete skel;
    return dblFiltered;

}

/* Compute the saliency for each skeleton point and threshold. Saliency is defined
   as in the paper:
 * Feature Preserving Smoothing of Shapes using Saliency Skeletons
 * (A. Telea, Visualization and Mathematics (Proc. VMLS'09), Springer, 2011, to appear)
 */
FIELD<float>* saliencyThreshold(FIELD<float>* distMap, FIELD<float> *grad) {
    ConnectedComponents *CC = new ConnectedComponents(2550);
    int                 *ccaOut = new int[grad->dimX() * grad->dimY()];
    int                 hLabel;
    unsigned int        *hist;
    FIELD<float>        *salskel = grad->dupe();
    int                 i,
                        nPix = salskel->dimX() * salskel->dimY();

    float               *dmd = distMap->data(),
                         *grd = grad->data(),
                          *ssd = salskel->data();

    /* Compute the saliency metric and threshold */
    i = 0;

    while (i < nPix) {
        ssd[i] = (dmd[i] < EPSILON) ? 0 : grd[i] / dmd[i];
        ssd[i] = ssd[i] < SKELETON_SALIENCY_THRESHOLD ? 0 : 255;
        ++i;

    }
    /* Perform connected component analysis */
    hLabel = CC->connected(ssd, ccaOut, grad->dimX(), grad->dimY(), std::equal_to<float>(), true);
    hist = (unsigned int *) SAFE_CALLOC((hLabel + 1) * sizeof (unsigned int));
    for (i = 0; i < nPix; i++) { hist[ccaOut[i]]++; }

    /* Remove small islands */
    for (i = 0; i < nPix; i++) {
       //ssd[i] = (hist[ccaOut[i]] >= SKELETON_ISLAND_THRESHOLD) ? ssd[i] : 0;
        ssd[i] = (hist[ccaOut[i]] >= SKELETON_ISLAND_THRESHOLD/100*sqrt(salskel->dimX()*salskel->dimY())) ? ssd[i] : 0;
    }
    // Wang. Didn't run to this function, use cuda instead. 
    delete CC;
    delete [] ccaOut;
    free(hist);
    return salskel;
}


FIELD<float>* do_one_pass(FIELD<float>* fi, FLAGS* flagsi, FIELD<std::multimap<float, int> >* origs, int dir, ModifiedFastMarchingMethod::METHOD meth) {
    FIELD<float>* f = (!dir) ? new FIELD<float>(*fi) : fi; //Copy input field
    FLAGS* flags = new FLAGS(*flagsi); //Copy flags field
    FIELD<float>* count = new FIELD<float>;

    ModifiedFastMarchingMethod *fmm = new ModifiedFastMarchingMethod(f, flags, count, origs); //Create fast marching method engine

    fmm->setScanDir(dir); //Set its various parameters
    fmm->setMethod(meth);

    int nfail, nextr;

    fmm->execute(nfail, nextr); //...and execute the skeletonization

    delete flags;
    if (!dir) { delete f; }
    delete fmm;
    return count;
}


void postprocess(FIELD<float>* grad, FIELD<float>* skel, float level)
//Simple reduction of gradient to binary skeleton via thresholding
{
    for (int j = 0; j < grad->dimY(); j++) //Threshold 'grad' to get real skeleton
        for (int i = 0; i < grad->dimX(); i++) {
            float g = grad->value(i, j);
            if (g > INFINITY / 2) { g = 1.0; }
            else if (g > level) { g = 255; }
            else { g = 0; }
            skel->value(i, j) = g;
        }
}

void comp_grad2(FIELD<float>* cnt1, FIELD<float>* cnt2, FIELD<float>* grad)
//Gradient computation using 2-pass method (i.e. grad computed out of 2 fields)
{
    int i, j;
    float MYINFINITY_2 = MYINFINITY / 2;

    for (j = 0; j < grad->dimY(); j++) //Compute grad in a special way, i.e. on a 2-pixel
        for (i = 0; i < grad->dimX(); i++) { //neighbourhood - this ensures pixel-size skeletons!
            float ux1 = cnt1->value(i + 1, j) - cnt1->value(i, j);
            float uy1 = cnt1->value(i, j + 1) - cnt1->value(i, j);
            float g1 = max(fabs(ux1), fabs(uy1));
            float ux2 = cnt2->value(i + 1, j) - cnt2->value(i, j);
            float uy2 = cnt2->value(i, j + 1) - cnt2->value(i, j);
            float g2 = max(fabs(ux2), fabs(uy2));
            grad->value(i, j) = (g1 > MYINFINITY_2 || g2 > MYINFINITY_2) ? 0 : min(g1, g2);
        }
}
