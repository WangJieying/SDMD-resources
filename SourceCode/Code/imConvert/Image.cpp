#include "include/Image.hpp"
#include <sys/stat.h>
#include <omp.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <set>
#include "include/ImageWriter.hpp"
#include "include/skeleton_cuda.hpp"
#include "include/connected.hpp"
#include "include/messages.h"
#include <parallel/algorithm>
#include <unordered_set>
#include <boost/functional/hash.hpp>
#include "SplineGenerate/BSplineCurveFitter/BSplineCurveFitterWindow3.h"


int EPSILON=0.00001;
string OUTPUT_FILE;
bool BUNDLE, OVERLAP_PRUNE, MSIL_SELECTION;
float ALPHA;
float OVERLAP_PRUNE_EPSILON;
int MIN_PATH_LENGTH;
int MIN_SUM_RADIUS;
int MIN_OBJECT_SIZE;
float SKELETON_ISLAND_THRESHOLD;
int peaks;
int distinguishable_interval;
FIELD<float>* skelImp;//for importance map
vector<vector<Vector3<float>>> BranchSet;
float diagonal, hausdorff;
ofstream OutFileB;
vector<int *> connection;
bool adaptive = true;//open the 'adaptive layer encoding'
//bool adaptive = false;
bool REMOVE_Y = false; //open 'Y-romval' or not.
int max_elem = 0;
int time1 = 0;
bool firstTime1 = true;
//float SkelTime=0, SplineTime = 0;
//ofstream Outfile;
/*************** CONSTRUCTORS ***************/
Image::Image(FIELD<float> *in, float islandThresh, float importanceThresh, int GrayInterval) {
    PRINT(MSG_NORMAL, "Creating Image Object...\n");
    this->layerThreshold = importanceThresh;
    this->DistinguishableInterval = GrayInterval;
    this->islandThreshold = islandThresh;
    this->importance = NULL;
    this->im = in;
    this->nPix = in->dimX() * in->dimY();
    std::set<int> levels(in->data(), in->data() + this->nPix);
    this->graylevels = reinterpret_cast<int*>(malloc(levels.size() * sizeof(int)));
    std::set<int>::iterator it = levels.begin();
    for (uint i = 0; i < levels.size(); ++i) {
        this->graylevels[i] = *it;
        std::advance(it, 1);
    }
    this->numLayers = levels.size();
    PRINT(MSG_NORMAL, "Done!\n");
}

Image::Image(FIELD<float> *in) {
    Image(in, 0, 0, 0);
}

Image::~Image() {
    deallocateCudaMem();
    free(importance);
    free(graylevels);
}

/*************** FUNCTIONS **************/
void detect_peak(
    const double*   data, /* the data */
    int             data_count, /* row count of data */
    vector<int>&    emi_peaks, /* emission peaks will be put here */
    double          delta, /* delta used for distinguishing peaks */
    int             emi_first /* should we search emission peak first of
                                     absorption peak first? */
) {
    int     i;
    double  mx;
    double  mn;
    int     mx_pos = 0;
    int     mn_pos = 0;
    int     is_detecting_emi = emi_first;


    mx = data[0];
    mn = data[0];

    for (i = 1; i < data_count; ++i) {
        if (data[i] > mx) {
            mx_pos = i;
            mx = data[i];
        }
        if (data[i] < mn) {
            mn_pos = i;
            mn = data[i];
        }

        if (is_detecting_emi &&
                data[i] < mx - delta) {

            emi_peaks.push_back(mx_pos);
            is_detecting_emi = 0;

            i = mx_pos - 1;

            mn = data[mx_pos];
            mn_pos = mx_pos;
        } else if ((!is_detecting_emi) &&
                   data[i] > mn + delta) {

            is_detecting_emi = 1;

            i = mn_pos - 1;

            mx = data[mn_pos];
            mx_pos = mn_pos;
        }
    }
}

void find_peaks(double* importance, double width) {
    double impfac = 0.1;
    vector<int> v;
    int numiters = 0;
    while (numiters < 1000) {
        v.clear();
        detect_peak(importance, 256, v, impfac, 0);
        if (v.size() < width)
            impfac *= .9;
        else if (v.size() > width)
            impfac /= .9;
        else
            break;
        if(impfac < 0.0002) break;
        numiters++;
    } 
    memset(importance, 0, 256 * sizeof(double));
    for (auto elem : v)
        importance[elem] = 1;
}

void detect_layers(int clear_color, double* upper_level, double threshold, bool needAssign)
{
    peaks = 0;
    int i = clear_color;
    int StartPoint = distinguishable_interval; //There is no need to check i and i+1; it is not distinguished by eyes, so check between i and i+StartPoint.
    double difference;

    double copy_upper_level[256];

    for (int j = 0; j < 256; ++j){
        copy_upper_level [j] = upper_level [j];
    }

    while ((i + StartPoint) < (max_elem + 1))
    {
        //difference = copy_upper_level[i] - copy_upper_level[i + StartPoint];///orig
        difference = copy_upper_level[i + StartPoint] - copy_upper_level[i];//attention: here shouldn't be upper_level
        if (difference > threshold)//choose this layer
        {
            if (needAssign) {
                upper_level[i + StartPoint] = 1;
            }
            
            i = i + StartPoint;
           // PRINT(MSG_NORMAL, "Choose_layers: %d\n",i);
            StartPoint = distinguishable_interval;
            peaks++; 
        }
        else
        {
            StartPoint += 1;
        }
        //cout<<"Choose_L: "<<i<<" StartPoint: "<<StartPoint<<" peaks "<<peaks<<" diff: "<<difference<<" thres: "<<threshold<<endl;   
    }

}

//binary search
void Image::find_layers(int clear_color, double* importance_upper, double width)
{
    double impfac = 0.5;
    double head = 0;
    double tail = 0.5;
    int numiters = 0;
    while (numiters < 100) {
        if(impfac < 0.003) break;//The difference is too small
        detect_layers(clear_color, importance_upper, impfac, 0);
        //PRINT(MSG_NORMAL, "the number of peaks: %d\n",peaks);
        if (peaks < width){//impfac need a smaller one
            tail = impfac;
            impfac = (head + impfac)/2;
        }
            
        else if (peaks > width) //impfac need a bigger one
            {
                head = impfac;
                impfac = (tail + impfac)/2;
            }
            else
             break;
        numiters++;
    }
   
    detect_layers(clear_color, importance_upper, impfac, 1);//the impfac to be calculated is 0.003/2
    
     for (int i = 0; i < 256; ++i) {
         if (importance_upper[i] == 1) {
             importance[i] = 1;
         }
         else {
            importance_upper[i] = 0;
            importance[i] = 0;
        }
    }
}

/*
* Calculate the histogram of the image, which is equal to the importance for each level.
* Avoid the use of in->value(), because it is less efficient (performs multiplications).
* The order is irrelevant anyway.
*/
void Image::calculateImportance() {
    PRINT(MSG_NORMAL, "Calculating the importance for each layer...\n");
    int normFac = 0;
    float *c = im->data();
    float *end = im->data() + nPix;

    int min_elem = 1e5;
    while (c != end){
        //if(*c >250) cout<<*c<<" ";
        max_elem = max(max_elem, *c);
        min_elem = min(min_elem, *c++);
    }
        
    clear_color = min_elem;
    cout<<"clear_color: "<<min_elem<<endl;

    /* If importance was already calculated before, cleanup! */
    if (importance) { free(importance); }
    importance = static_cast<double*>(calloc(256, sizeof(double)));
    double* UpperLevelSet = static_cast<double*>(calloc(256, sizeof(double)));

    if (!importance) {
        PRINT(MSG_ERROR, "Error: could not allocate importance histogram\n");
        exit(-1);
    }

    c = im->data();//back to the beginning!
    // Create a histogram
    while (c < end) {
        importance[static_cast<int>(*c++)] += 1;
    }

    ////////upper_level set histogram/////////
    //orig///
 /*    UpperLevelSet[255] = importance[255];
    for (int i = 255; i > 0; i--) 
        UpperLevelSet[i-1] = importance[i-1] + UpperLevelSet[i];
*/
   //new
    UpperLevelSet[0] = importance[0];
    for (int i = 0; i < 255; i++)
        UpperLevelSet[i+1] = importance[i+1] + UpperLevelSet[i];


    // Normalize. Local-maximal method.
    normFac = static_cast<int>(*std::max_element(importance, importance + 256));//find the max one.
    for (int i = 0; i < 256; ++i) {
        importance[i] /= static_cast<double>(normFac);
    }
    // Normalize. Cumulative method.
    //double max = UpperLevelSet[0];///orig
    double max = UpperLevelSet[255];
    for (int i = 0; i < 256; ++i) {
        UpperLevelSet[i] = UpperLevelSet[i] / static_cast<double>(max) - EPSILON;//To avoid to be 1.
    }

    // Cumulative method.
    if (MSIL_SELECTION) {
        distinguishable_interval = DistinguishableInterval;
        find_layers(clear_color, UpperLevelSet, layerThreshold);
        //memcpy(importance, UpperLevelSet, sizeof(UpperLevelSet));
        //importance = UpperLevelSet; // Danger. if free(UpperLevelSet); then importance will be weird.
    } 
    else {// else local-maxima method
        find_peaks(importance, layerThreshold);
    }

    importance[clear_color] = 1;
    
    std::vector<int> v;
    for (int i = 0; i < 256; ++i) {
        if (importance[i] == 1) {
            v.push_back(i);
        }
    }
    if (v.size() == 0) {
        PRINT(MSG_ERROR, "ERROR: No layers selected. Exiting...\n");
        exit(-1);
    }
    PRINT(MSG_NORMAL, "Selected %lu layers: ", v.size());
    std::ostringstream ss;

    std::copy(v.begin(), v.end() - 1, std::ostream_iterator<int>(ss, ", "));
    ss << v.back();
    PRINT(MSG_NORMAL, "%s\n", ss.str().c_str());
    v.clear();
    max_elem = 0;
    free(UpperLevelSet);
}

/**
* fullDilate and fullErode are placeholders. Although they do work, they
* should be replaced by better erode and dilation functions. This is used primarily
* to test how much an opening on the skeleton will reduce the image size.
*/

/* fullDilate -- Perform dilation with a S.E. of 3x3, filled with ones. */
FIELD<float> * fullDilate(FIELD<float> *layer) {
    FIELD<float> *ret = new FIELD<float>(layer->dimX(), layer->dimY());
    memset(ret->data(), 0, layer->dimX() * layer->dimY() * sizeof(float));
    for (int y = 0; y < layer->dimY(); ++y) {
        for (int x = 0; x < layer->dimX(); ++x) {
            if (layer->value(x, y)) {
                ret->set(x - 1, y - 1, 255);
                ret->set(x - 1, y  , 255);
                ret->set(x - 1, y + 1, 255);
                ret->set(x  , y - 1, 255);
                ret->set(x  , y  , 255);
                ret->set(x  , y + 1, 255);
                ret->set(x + 1, y - 1, 255);
                ret->set(x + 1, y  , 255);
                ret->set(x + 1, y + 1, 255);
            }
        }
    }
    delete layer;
    return ret;
}

/* fullErode -- Perform erosion with a S.E. of 3x3, filled with ones. */
FIELD<float> * fullErode(FIELD<float> *layer) {
    FIELD<float> *ret = new FIELD<float>(layer->dimX(), layer->dimY());
    for (int y = 0; y < layer->dimY(); ++y) {
        for (int x = 0; x < layer->dimX(); ++x) {
            if (
                layer->value(x - 1, y - 1) &&
                layer->value(x - 1, y) &&
                layer->value(x - 1, y + 1) &&
                layer->value(x, y - 1) &&
                layer->value(x, y) &&
                layer->value(x, y + 1) &&
                layer->value(x + 1, y - 1) &&
                layer->value(x + 1, y) &&
                layer->value(x + 1, y + 1)
            ) {
                ret->set(x, y, 255);
            } else { ret->set(x, y, 0); }
        }
    }
    delete layer;
    return ret;
}

/* rmObject -Remove current object in a 3x3 kernel, used for removeDoubleSkel: */
void rmObject(int *k, int x, int y) {
    if (x < 0 || x > 2 || y < 0 || y > 2 || k[y * 3 + x] == 0) { return; }
    k[y * 3 + x] = 0;
    rmObject(k, x + 1, y + 1);
    rmObject(k, x + 1, y);
    rmObject(k, x + 1, y - 1);
    rmObject(k, x, y + 1);
    rmObject(k, x, y - 1);
    rmObject(k, x - 1, y + 1);
    rmObject(k, x - 1, y);
    rmObject(k, x - 1, y - 1);
}
/* numObjects - Count the number of objects in a 3x3 kernel, used for removeDoubleSkel: */
int numObjects(int *k) {
    int c = 0;
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; ++y) {
            if (k[y * 3 + x]) { rmObject(k, x, y); c++; }
        }
    }
    return c;
}

/**
* removeDoubleSkel
* @param FIELD<float> * layer -- the layer of which the skeleton should be reduced
* @return new FIELD<float> *. Copy of 'layer', where all redundant skeleton-points are removed (i.e. rows of width 2.)
*/
FIELD<float> * removeDoubleSkel(FIELD<float> *layer) {
    int *k = (int *)calloc(9, sizeof(int));
    for (int y = 0; y < layer->dimY(); ++y) {
        for (int x = 0; x < layer->dimX(); ++x) {
            if (layer->value(x, y)) {
                k[0] = layer->value(x - 1, y - 1);
                k[1] = layer->value(x - 1, y);
                k[2] = layer->value(x - 1, y + 1);
                k[3] = layer->value(x  , y - 1);
                k[4] = 0;
                k[5] = layer->value(x  , y + 1);
                k[6] = layer->value(x + 1, y - 1);
                k[7] = layer->value(x + 1, y);
                k[8] = layer->value(x + 1, y + 1);
                if (k[0] + k[1] + k[2] + k[3] + k[4] + k[5] + k[6] + k[7] + k[8] > 256) {
                    int b = numObjects(k);
                    if (b < 2 ) //{layer->set(x, y, 0); } //original method. Has a problem.
                    { //wang - change a bit.
                        if(layer->value(x + 1, y) && layer->value(x + 1, y + 1)) layer->set(x+1, y, 0); //k[7] && k[8]
                        else layer->set(x, y, 0);
                    }
                }
            }
        }
    }
    free(k);
    return layer;
}


void NewRemoveDoubleSkel(FIELD<float> * skelImp)
{
    //minimalSkel = new FIELD<float>(skelImp->dimX(), skelImp->dimY());
    int *k = (int *)calloc(9, sizeof(int));
    int thres = 8;
    for (int y = 0; y < skelImp->dimY(); ++y) {
        for (int x = 0; x < skelImp->dimX(); ++x) {
            if (skelImp->value(x, y)) {
                k[0] = skelImp->value(x - 1, y - 1);
                k[1] = skelImp->value(x - 1, y);
                k[2] = skelImp->value(x - 1, y + 1);
                k[3] = skelImp->value(x  , y - 1);
                k[4] = skelImp->value(x  , y);
                k[5] = skelImp->value(x  , y + 1);
                k[6] = skelImp->value(x + 1, y - 1);
                k[7] = skelImp->value(x + 1, y);
                k[8] = skelImp->value(x + 1, y + 1);
               
                if (k[0]) {// if (lower neighbor < 2 * lower left neighbor && lower neighbor < 2 * central point), then remove the lower neighbor; 
                    if(k[3] && k[3] < 2*k[0] && k[3] < 2*k[4]) skelImp->set(x, y-1, 0);
                    if(k[1] && k[1] < 2*k[0] && k[1] < 2*k[4]) skelImp->set(x-1, y, 0);
                }
                if(k[2]) {
                    if(k[1] && k[1] < 2*k[2] && k[1] < 2*k[4]) skelImp->set(x-1, y, 0);
                    if(k[5] && k[5] < 2*k[2] && k[5] < 2*k[4]) skelImp->set(x, y+1, 0);
                }
                if(k[6]) {
                    if(k[3] && k[3] < 2*k[6] && k[3] < 2*k[4]) skelImp->set(x, y-1, 0);
                    if(k[7] && k[7] < 2*k[6] && k[7] < 2*k[4]) skelImp->set(x+1, y, 0);
                }
                if(k[8]) {
                    if(k[7] && k[7] < 2*k[8] && k[7] < 2*k[4]) skelImp->set(x+1, y, 0);
                    if(k[5] && k[5] < 2*k[8] && k[5] < 2*k[4]) skelImp->set(x, y+1, 0);
                }
                /*
                if(k[5])
                    if(k[5] < thres) skelImp->set(x, y+1, 0);
                    else if(k[8] > thres1 && k[4] > thres1) skelImp->set(x, y+1, 0);
                */
            }
        }
    }
    free(k);
    //return layer;

}


/*
* Remove small islands according the the islandThreshold variable. Notice that both "on" and "off"
* regions will be filtered.
*/
void Image::removeIslands() {
    int i, j, k;                    /* Iterators */
    FIELD<float> *inDuplicate = 0;  /* Duplicate, because of inplace modifications */
    FIELD<float> *newImg = new FIELD<float>(im->dimX(), im->dimY());
    int highestLabel;               /* for the CCA */
    int *ccaOut;                    /* labeled output */
    ConnectedComponents *CC;        /* CCA-object */
    float *fdata;
    unsigned int *hist;
    
    PRINT(MSG_NORMAL, "Removing small islands...\n");
    /* Some basic initialization */
    memset(newImg->data(), 0, nPix * sizeof(float));

    /* Connected Component Analysis */
    #pragma omp parallel for private(i, j, k, ccaOut, CC, fdata, highestLabel, hist, inDuplicate)

    
    for (i = 0; i < 0xff; ++i) {
       // PRINT(MSG_VERBOSE, "Layer: %d\n", i);
        // The below value refers to the expected number of components in an image.
        CC = new ConnectedComponents(255);
        ccaOut = new int[nPix];

        inDuplicate = (*im).dupe();
        inDuplicate->threshold(i);//threshold-set..
        ////////debug
    /*if(i==52){
        if (firstTime1)
           inDuplicate->writePGM("gray52.pgm");
    }*/
        
        fdata = inDuplicate->data();
 
        /* CCA -- store highest label in 'max' -- Calculate histogram */
        highestLabel = CC->connected(fdata, ccaOut, im->dimX(), im->dimY(), std::equal_to<float>(), true);//true is 8-connect.
        hist = static_cast<unsigned int*>(calloc(highestLabel + 1, sizeof(unsigned int)));
        if (!hist) {
            PRINT(MSG_ERROR, "Error: Could not allocate histogram for connected components\n");
            exit(-1);
        }
        for (j = 0; j < nPix; j++) { hist[ccaOut[j]]++; }

       
        //cout<<"islandthreshold "<<islandThreshold<<endl;
        /* Remove small islands */
        for (j = 0; j < nPix; j++) {
           fdata[j] = (hist[ccaOut[j]] >= (islandThreshold/100*im->dimX()*im->dimY())) ? fdata[j] : 255 - fdata[j]; //change the absolute num. to %
             //fdata[j] = (hist[ccaOut[j]] >= islandThreshold) ? fdata[j] : 255 - fdata[j]; //change the absolute num. to %
        }
        
        #pragma omp critical
        {
            for (j = 0; j < im->dimY(); j++)
                for (k = 0; k < im->dimX(); k++)
                    if (0 == fdata[j * im->dimX() + k] && newImg->value(k, j) < i) { newImg->set(k, j, i); }
        }

        
        /* Cleanup */
        free(hist);
        delete [] ccaOut;
        delete CC;
        delete inDuplicate;
    }

    for (j = 0; j < im->dimY(); j++)
        for (k = 0; k < im->dimX(); k++)
            im->set(k, j, newImg->value(k, j));

    delete newImg;
    PRINT(MSG_NORMAL, "Done!\n");
}

FIELD<float>* Image::removeIslands2(FIELD<float>* img){
    int i, j;                    /* Iterators */
    int highestLabel;               /* for the CCA */
    int *ccaOut;                    /* labeled output */
    ConnectedComponents *CC;        /* CCA-object */
    float *fdata;
    unsigned int *hist;
    

    /* Connected Component Analysis */
    //#pragma omp parallel for private(i, j, ccaOut, CC, fdata, highestLabel, hist)

    // The below value refers to the expected number of components in an image.
    CC = new ConnectedComponents(255);
    ccaOut = new int[nPix];

    fdata = img->data();

    /* CCA -- store highest label in 'max' -- Calculate histogram */
    highestLabel = CC->connected(fdata, ccaOut, im->dimX(), im->dimY(), std::equal_to<float>(), true);//true is 8-connect.
    hist = static_cast<unsigned int*>(calloc(highestLabel + 1, sizeof(unsigned int)));
    if (!hist) {
        PRINT(MSG_ERROR, "Error: Could not allocate histogram for connected components\n");
        exit(-1);
    }
    for (j = 0; j < nPix; j++) { hist[ccaOut[j]]++; }

    
    /* Remove small islands */
    for (j = 0; j < nPix; j++) 
        fdata[j] = (hist[ccaOut[j]] >= (islandThreshold/100*im->dimX()*im->dimY())) ? fdata[j] : 255 - fdata[j]; //change the absolute num. to %
            
    
    FIELD<float>* f = new FIELD<float>(im->dimX(), im->dimY());
    
    for (i = 0; i < im->dimX(); ++i) {
        for (j = 0; j < im->dimY(); ++j) {
            
            f->set(i, j, fdata[j * im->dimX() + i]);
        }
    }
    //f->writePGM("afterIsland_52.pgm");
    

    free(hist);
    delete [] ccaOut;
    delete CC;
       
    return f;
}

/**
* Remove unimportant layers -- Filter all layers for which their importance is lower than layerThreshold
*/
void Image::removeLayers() {
    float val_up, val_dn;

    PRINT(MSG_NORMAL, "Filtering image layers...\n");

    for (int y = 0; y < im->dimY(); y++) {
        for (int x = 0; x < im->dimX(); x++) {
            val_up = im->value(x, y);
            val_dn = im->value(x, y);
            if (importance[(int)im->value(x, y)] == 1)
                continue;
            while (val_dn >= 0 || val_up <= 256) {
                if (val_dn >= 0) {
                    if (importance[(int)val_dn] == 1) {
                        im->set(x, y, val_dn);
                        break;
                    }
                }
                if (val_up <= 256) {
                    if (importance[(int)val_up] == 1) {
                        im->set(x, y, val_up);
                        break;
                    }
                }
                val_dn--;
                val_up++;
                // if (val_up < 256) {
                // }
            }
        }
    }
    
}

void draw_path(int intensity, FIELD<float>* image, skel_tree_t *st, uint16_t pLength, bool rightMost) {
    /* Not a leaf, does it have exactly 1 child (is it a continuous path?) ?*/
    if (st->numChildren() == 0) {
        Triple<int, int, int> p = st->getValue();
        image->set(p.first, p.second, intensity);
    }

    if (st->numChildren() == 1) {
        Triple<int, int, int> p = st->getValue();
        image->set(p.first, p.second, intensity);
        // add_to_hist(st->getValue(), st->getChild(0)->getValue());
        draw_path(intensity, image, st->getChild(0), pLength + 1, rightMost);
        return;
    }

    /* Fork coming up! */
    if (st->numChildren() > 1) {
        /* All "non-rightmost" children: */
        auto cur = st->getValue();
        image->set(cur.first, cur.second, intensity);

        for (int i = 0; i < (st->numChildren() - 1) ; ++i) {
            draw_path(intensity, image, st->getChild(i), 1, false);
        }
        /* Treat rightmost child different, pass a longer path length, so it jumps back further after being done with the last branch. */
        // add_to_hist(cur, st->getChild(st->numChildren() - 1)->getValue());
        draw_path(intensity, image, st->getChild(st->numChildren() - 1), 1 + pLength, rightMost);
    }
}


void draw_layer(int intensity, skel_tree_t* trees, FIELD<float>* image) {
    /* Remove overhead for empty layers. */
    if (trees->numChildren() == 0) { return; }

    /* Top layer are disjunct paths. Treat them as separate objects */
    for (int child = 0; child < trees->numChildren(); ++child) {
        skel_tree_t *curnode = (*trees)[child];
        draw_path(intensity, image, curnode, /*pLength = */1, /*rightMost = */true);
    }
}

coord2D_list_t * neighbours(int x, int y, FIELD<float> *skel) {
    coord2D_list_t *neigh = new coord2D_list_t();
    int n[8] = {1, 1, 1, 1, 1, 1, 1, 1};

    // Check if we are hitting a boundary on the image 
    if (x <= 0 )             {        n[0] = 0;        n[3] = 0;        n[5] = 0;    }
    if (x >= skel->dimX() - 1) {        n[2] = 0;        n[4] = 0;        n[7] = 0;    }
    if (y <= 0)              {        n[0] = 0;        n[1] = 0;        n[2] = 0;    }
    if (y >= skel->dimY() - 1) {        n[5] = 0;        n[6] = 0;        n[7] = 0;    }

    // For all valid coordinates in the 3x3 region: check for neighbours
    if ((n[0] != 0) && (skel->value(x - 1, y - 1) > 0)) { neigh->push_back(coord2D_t(x - 1, y - 1)); }
    if ((n[1] != 0) && (skel->value(x    , y - 1) > 0)) { neigh->push_back(coord2D_t(x    , y - 1)); }
    if ((n[2] != 0) && (skel->value(x + 1, y - 1) > 0)) { neigh->push_back(coord2D_t(x + 1, y - 1)); }
    if ((n[3] != 0) && (skel->value(x - 1, y    ) > 0)) { neigh->push_back(coord2D_t(x - 1, y    )); }
    if ((n[4] != 0) && (skel->value(x + 1, y    ) > 0)) { neigh->push_back(coord2D_t(x + 1, y    )); }
    if ((n[5] != 0) && (skel->value(x - 1, y + 1) > 0)) { neigh->push_back(coord2D_t(x - 1, y + 1)); }
    if ((n[6] != 0) && (skel->value(x    , y + 1) > 0)) { neigh->push_back(coord2D_t(x    , y + 1)); }
    if ((n[7] != 0) && (skel->value(x + 1, y + 1) > 0)) { neigh->push_back(coord2D_t(x + 1, y + 1)); }

    return neigh;
}


skel_tree_t *tracePath(int x, int y, FIELD<float> *skel, FIELD<float> *dt) {
    coord2D_t n;
    coord2D_list_t *neigh;
    skel_tree_t *path;
    if (skel->value(x, y) == 0) {
        PRINT(MSG_ERROR, "Reached invalid point.\n");
        return NULL;
    }

    /* Create new node and add to root */
    path = new SkelNode<coord3D_t>(coord3D_t(x, y, dt->value(x, y)));
    skel->set(x, y, 0);

    neigh = neighbours(x, y, skel);
    /* Add children */
    while (neigh->size() > 0) {
        n = *(neigh->begin());
        path->addChild(tracePath(n.first, n.second, skel, dt));
        delete neigh;
        neigh = neighbours(x, y, skel);
    }

    delete neigh;
    return path;
}


void tracePath(int x, int y, FIELD<float> *skel, FIELD<float> *dt, int &seq) {
    coord2D_t n, list1;
    coord2D_list_t *neigh, *neigh_;
    coord2D_list_t neighList; //vector<pair<int, int>>
    coord2D_list_t *SetZero = new coord2D_list_t();

	//	OutFile<<seq<<" " << x << " " << y << " " <<dt->value(x, y)<< " " <<skelImp->value(x,y) <<endl;//for 4D
    OutFileB<<seq<<" " << x << " " << y << " " <<dt->value(x, y)<<endl;//for 3D
	skel->set(x, y, 0);
    //segment->set(x,y,255);
	neigh = neighbours(x, y, skel);

	if (neigh->size() < 1) ;//==0; at end.
	else if (neigh->size() < 2)//==1; same branch.
	{
		n = *(neigh->begin());
		tracePath(n.first, n.second, skel, dt, seq);//same seq;
	}
	else if (neigh->size() > 1) {//produce branches.
		//seqT=seq;
		while (neigh->size() > 0) {
			seq++;
			
			n = *(neigh->begin());
            int first = n.first;
            int second = n.second;
            neigh->erase(neigh->begin());

            for (auto i = neigh->begin(); i!=neigh->end();i++){
                skel->set((*i).first,(*i).second, 0);//first set to 0
                if(abs(first-(*i).first) + abs(second-(*i).second)==1)//is nerghbor
                {
                    neigh_ = neighbours((*i).first, (*i).second, skel);
                    for (auto i_ = neigh_->begin();i_!=neigh_->end();i_++)
                    {
                        if ((*i_).first==first && (*i_).second==second) continue;
                        else
                        {
                            if(abs(skelImp->value((*i_).first,(*i_).second)-skelImp->value((*i).first,(*i).second))<5)
                            {
                                skel->set((*i_).first,(*i_).second,0);
                                SetZero->push_back(coord2D_t((*i_).first, (*i_).second));
                            }
                        }
                    }
                

                }
            }
                
			tracePath(first, second, skel, dt, seq);

            for (auto i = neigh->begin(); i!=neigh->end();i++)
                skel->set((*i).first,(*i).second, 1);//then set back to 1.

            for (auto i = SetZero->begin(); i!=SetZero->end();i++)
                skel->set((*i).first,(*i).second, 1);
            SetZero->clear();
			//delete neigh;
			//neigh = neighbours(x, y, skel);
    	}
	}
	 delete neigh;
     delete neigh_;		

}

//original method.
void tracePath(int x, int y, FIELD<float> *skel, FIELD<float> *dt, vector<Vector3<float>> Branch, int &seq){
    int *EachConnect = (int *)calloc(4, sizeof(int));
    int index = 0;
    Vector3<float> CurrentPx;
    coord2D_t n;
    coord2D_list_t *neigh;
    //coord2D_list_t *neigh_; //vector<pair<int, int>>
    //coord2D_list_t *SetZero = new coord2D_list_t();

    CurrentPx[0] = (float)x/diagonal;
    CurrentPx[1] = (float)y/diagonal;
    CurrentPx[2] = (float)dt->value(x, y)/diagonal;

	
    Branch.push_back(CurrentPx);
    
	skel->set(x, y, 0);
    //segment->set(x,y,255);
	neigh = neighbours(x, y, skel);

	if (neigh->size() < 1) BranchSet.push_back(Branch);//==0; at end.
	else if (neigh->size() < 2)//==1; same branch.
	{
		n = *(neigh->begin());
		tracePath(n.first, n.second, skel, dt, Branch,seq);//same seq;
	}
	else if (neigh->size() > 1) {//produce branches.
        //if (Branch.size()>1)	
        BranchSet.push_back(Branch);
        EachConnect[index] = seq;


		while (neigh->size() > 0) {
			seq++;
            index++;
			n = *(neigh->begin());
            int first = n.first;
            int second = n.second;
            neigh->erase(neigh->begin());

            for (auto i = neigh->begin(); i!=neigh->end();i++){//See the explainaton in my note
                skel->set((*i).first,(*i).second, 0);//first set to 0
            }
            EachConnect[index] = seq;
            vector<Vector3<float>> NewBranch;
			tracePath(first, second, skel, dt, NewBranch,seq);

            for (auto i = neigh->begin(); i!=neigh->end();i++)
                skel->set((*i).first,(*i).second, 1);//then set back to 1.
            
			//delete neigh;
			//neigh = neighbours(x, y, skel);
    	}
        connection.push_back(EachConnect);
	}
	 delete neigh;
     //free(EachConnect);  
}

skel_tree_t* traceLayer(FIELD<float> *skel, FIELD<float> *dt) {
    skel_tree_t *root;
    coord3D_t rootCoord = coord3D_t(-1, -1, -1);
    root = new skel_tree_t( rootCoord );
    for (int y = 0; y < skel->dimY(); ++y) {
        for (int x = 0; x < skel->dimX(); ++x) {
            if (skel->value(x, y) > 0) {
                root->addChild(tracePath(x, y, skel, dt));
            }
        }
    }
    return root;
}

void bundle(FIELD<float>* skelCurr, FIELD<float>* currDT, FIELD<float>* prevDT, short* prev_skel_ft, int fboSize) {
    ALPHA = max(0, min(1, ALPHA));
    for (int i = 0; i < skelCurr->dimX(); ++i) {
        for (int j = 0; j < skelCurr->dimY(); ++j) {
            if (skelCurr->value(i, j) > 0) {
                // We might bundle this point
                int id = j * fboSize + i;

                // Find closest point
                int closest_x = prev_skel_ft[2 * id];
                int closest_y = prev_skel_ft[2 * id + 1];
                if (closest_x == -1 && closest_y == 0)
                    continue;
                // If it is already on a previous skeleton point, were done
                double distance = sqrt((i - closest_x) * (i - closest_x) + (j - closest_y) * (j - closest_y));
                if (distance == 0.0)
                    continue;

                // Create vector from the current point to the closest prev skel point
                double xvec = (closest_x - i);
                double yvec = (closest_y - j);

                // What would be the location with no bound on the shift?
                int trans_vec_x = ALPHA * xvec;
                int trans_vec_y = ALPHA * yvec;
                double trans_vec_len = sqrt(trans_vec_x * trans_vec_x + trans_vec_y * trans_vec_y);

                // If we don't move it at all were done
                if (trans_vec_len == 0.0)
                    continue;

                // Find the limited length of this vector
                double desired_length = min(trans_vec_len, EPSILON);
                // Rescale the vector such that it is bounded by epsilon
                int new_x_coor = round(i + (trans_vec_x / trans_vec_len) * desired_length);
                int new_y_coor = round(j + (trans_vec_y / trans_vec_len) * desired_length);
                // Verify that the new distance is at most near EPSILON (i.e. FLOOR(new_distance) <= EPSILON)
                double new_distance = sqrt((i - new_x_coor) * (i - new_x_coor) + (j - new_y_coor) * (j - new_y_coor));
                PRINT(MSG_VERBOSE, "%d: (%d, %d) -> (%d, %d) (%lf) -> (%d, %d) (%lf)\n", EPSILON, i, j, closest_x, closest_y, distance, new_x_coor, new_y_coor, new_distance);

                // compute a new radius
                double new_r = min(currDT->value(i, j), prevDT->value(new_x_coor, new_y_coor));
                // double new_r = currDT->value(i, j);
                skelCurr->set(i, j, 0);
                skelCurr->set(new_x_coor, new_y_coor, 255);
                currDT->set(new_x_coor, new_y_coor, new_r);
            }
        }
    }
}

double overlap_prune(FIELD<float>* skelCurr, FIELD<float>* skelPrev, FIELD<float>* currDT, FIELD<float>* prevDT) {
    // Bigger at same location
    int count = 0, total_skel_points = 0;
    for (int i = 0; i < skelPrev->dimX(); ++i) {
        for (int j = 0; j < skelPrev->dimY(); ++j) {
            // If there is a skeleton point...
            if (skelCurr->value(i, j) > 0 && skelPrev->value(i, j) > 0) {
                // cout << skelCurr->value(i, j) << ", " << skelPrev->value(i, j) << endl;
                total_skel_points++;
                // ...and the radius difference w.r.t. the next layer is small enough...
                if ((currDT->value(i, j)) >= (prevDT->value(i, j))) {
                    // ...we can delete the skeleton point
                    skelPrev->set(i, j, 0);
                    count++;
                }
            }
        }
    }
// Return overlap prune ratio for statistics
    if (total_skel_points == 0)
        return 0;
    double overlap_prune_ratio = static_cast<double>(count) / total_skel_points * 100.0;
    (MSG_VERBOSE, "Overlap pruned %d points out %d points (%.2f%%)\n", count, total_skel_points, overlap_prune_ratio);

    return overlap_prune_ratio;
}

void removeSmallPaths(skel_tree_t *st) {
    if (st->numChildren() > 1) {
        for (int i = 0; i < st->numChildren() && st->numChildren() > 1; ++i) {
            if (st->getChild(i)->numRChildren() < MIN_PATH_LENGTH || st->getChild(i)->importance() < MIN_SUM_RADIUS) {
                st->removeRChild(i);
                --i;
            }
        }
    }

    for (int i = 0; i < st->numChildren(); ++i) {
        removeSmallPaths(st->getChild(i));
    }
}


void removeSmallObjects(skel_tree_t *st) {
    for (int i = 0; i < st->numChildren(); ++i) {
        if (st->getChild(i)->numRChildren() < MIN_OBJECT_SIZE || st->getChild(i)->importance() < MIN_SUM_RADIUS) {
            st->removeRChild(i);
            --i;
        }
    }
}

/* Due to some preprocessing steps which simplify the skeleton we could generate "new points", Points which are
 actually outside of an object may now become skeleton points (e.g. by dilation). These points have a radius of
 0. If these points are at the end of a skeleton path, we can safely remove them.*/
void removeInvisibleEndpoints(skel_tree_t *st) {
    for (int i = 0; i < st->numChildren(); ++i) {
        removeInvisibleEndpoints(st->getChild(i));

        if (st->getChild(i)->numRChildren() == 0 && st->getChild(i)->getValue().third == 0) {
            st->removeRChild(i);
            //cout<<"See how many invisible endpoints are removed."<<endl; //So Many!
            --i;
        }
    }
}


void filterTree(skel_tree_t *st) {
    removeInvisibleEndpoints(st);
    // We can never delete if they're both zero, so we can safely return then
    if (!(MIN_PATH_LENGTH == 0 && MIN_SUM_RADIUS == 0))
        removeSmallPaths(st);
    // We can never delete if they're both zero, so we can safely return then
    if (!(MIN_OBJECT_SIZE == 0 && MIN_SUM_RADIUS == 0))
        removeSmallObjects(st);
}

void trace_layer(vector<std::pair<int, skel_tree_t*>>* forest, int level, FIELD<float>* skel, FIELD<float>* dt) {
    skel_tree_t* paths = traceLayer(skel, dt);
    forest->push_back(make_pair(level, paths));
    //should delete paths?
}

void flatten_tree(skel_tree_t* tree, unordered_set<pair<int, int>, boost::hash< std::pair<int, int>>>& flat_map) {
    if (tree == nullptr || tree->numChildren() == 0)
        return;
    auto val = tree->getValue();
    flat_map.insert(make_pair(val.first, val.second));
    for (int i = 0; i < tree->numChildren(); ++i)
        flatten_tree(tree->getChild(i), flat_map);
}

void post_process(FIELD<float>* skel, FIELD<float>* DT) {
    skel = removeDoubleSkel(skel);
    auto skel_prev_dupe = skel->dupe();

    skel_tree_t* forest = traceLayer(skel_prev_dupe, DT);
    delete skel_prev_dupe;

    // TODO: This procedure of filtering is horribly slow, but necessary
    // Preferably this ought to be fixed in some way...
    filterTree(forest);
     unordered_set<pair<int, int>, boost::hash< pair<int, int>>> skel_pixels;

    // Delete all skeleton points from the image that are too short
    flatten_tree(forest, skel_pixels);
    // Delete the forest
    delete forest;
      for (int i = 0; i < skel->dimX(); ++i) {
        for (int j = 0; j < skel->dimY(); ++j) {
            if (skel_pixels.find(make_pair(i, j)) == skel_pixels.end()) {
                skel->set(i, j, 0);
            }
        }
    }
 /**/
}

void cool_effects(FIELD<float>* skel_curr, FIELD<float>* currDT, FIELD<float>* prevDT, short * prev_skel_ft, int fboSize) {
    if (BUNDLE) {
        if (prev_skel_ft) {
            bundle(skel_curr, currDT, prevDT, prev_skel_ft, fboSize);
        }
    }
}

void analyze_cca(FIELD<float>* skel) {

    float* ssd = skel->data();
    
    ConnectedComponents *CC = new ConnectedComponents(255);
    int                 *ccaOut = new int[skel->dimX() * skel->dimY()];
    int                 hLabel;
    unsigned int        *hist;
    int                 i,
                        nPix = skel->dimX() * skel->dimY();

    /* Perform connected component analysis */
    hLabel = CC->connected(ssd, ccaOut, skel->dimX(), skel->dimY(), std::equal_to<float>(), true);
    hist = static_cast<unsigned int*>(calloc(hLabel + 1, sizeof(unsigned int)));    
    if (!hist) {
        PRINT(MSG_ERROR, "Error: Could not allocate histogram for skeleton connected components\n");
        exit(-1);
    }
    //for (i = 0; i < hLabel; i++) { hist[i] = 0; /*if (hist[i] != 0) {/*cout<<hist[i]<<endl; counNonZero =+ 1;}*/ }
 
    for (i = 0; i < nPix; i++) { hist[ccaOut[i]]++; }
    
    /* Remove small islands */
    for (i = 0; i < nPix; i++) {
        ssd[i] = (hist[ccaOut[i]] >= SKELETON_ISLAND_THRESHOLD/100*sqrt(skel->dimX()*skel->dimY())) ? ssd[i] : 0;
       
    }

    free(hist);
    delete CC;
    delete [] ccaOut;

    skel->setData(ssd);
    //skel->writePGM("skel_removeIsland.pgm");
  
}

int Image:: CalculateCPnum(int i, FIELD<float> *imDupeCurr, int WriteToFile)
{
    FIELD<float> *skelCurr = 0;
    int seq = 0; 
    int CPnum = 0;
    bool print=false;
    
    skelImp = computeSkeleton(imDupeCurr, print);
    
    //NewRemoveDoubleSkel();
    //importance map or saliency map transforms to skel.
    skelCurr = new FIELD<float>(skelImp->dimX(), skelImp->dimY());
    for (int i = 0; i < skelImp->dimX(); ++i) {
        for (int j = 0; j < skelImp->dimY(); ++j) {
            bool is_skel_point = skelImp->value(i,j);
            skelCurr->set(i, j, is_skel_point ? 255 : 0);
        }
    } 
    
    analyze_cca(skelCurr);
    //post_process(skelCurr, imDupeCurr);
     skelCurr = removeDoubleSkel(skelCurr);
    for (int y = 0; y < skelCurr->dimY(); ++y) {
        for (int x = 0; x < skelCurr->dimX(); ++x) {
            if (skelCurr->value(x, y) > 0) {
                if(imDupeCurr->value(x,y) == 0)//dt=0
                    skelCurr->set(x,y,0);//get the connection
            }
        }
    }
     //debug--skel
  /* if(WriteToFile>0){
        stringstream skel;
        skel<<"skel"<<time1<<"-"<<i<<".pgm";
        skelCurr->writePGM(skel.str().c_str());
          
    }
     */ 
    ///////new method-----segment and store into the BranchSets;
    for (int y = 0; y < skelCurr->dimY(); ++y) {
        for (int x = 0; x < skelCurr->dimX(); ++x) {
            if (skelCurr->value(x, y) > 0) {
                vector<Vector3<float>> Branch;
                //if(imDupeCurr->value(x,y) == 0) cout<<"! ";
                tracePath(x, y, skelCurr, imDupeCurr, Branch, seq);//get the connection///
                seq++;
            }
        }
    }
    //seq = 0;
            ////fit with spline///
    //cout<<"BranchSet.size()--"<<BranchSet.size()<<endl;
     //begin = std::chrono::steady_clock::now();
 
    if(BranchSet.size()>0){
        BSplineCurveFitterWindow3 spline;
        if (WriteToFile>0) spline.SplineFit2(BranchSet, hausdorff, diagonal, i, connection, WriteToFile);
        else CPnum = spline.SplineFit(BranchSet, hausdorff, diagonal, i, connection);
        BranchSet.clear();//important.
        connection.clear();
    }
    //end = std::chrono::steady_clock::now();
    //diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    //SplineTime += diff_ms;
    
/* */
    delete skelCurr;
    return CPnum;
}

int Image::GetSkelandCPs(int level, FIELD<float> * ExtendedLayer, int topExtension, int leftExtension, int WriteToFile) {
    int i,j;
    //initialize_skeletonization(ExtendedLayer);
    FIELD<float> *skelImp = computeSkeleton(ExtendedLayer, 0);//after this func, ExtendedLayer magically transforms into the DT map
    
    FIELD<float> *skelLarger = new FIELD<float>(skelImp->dimX(), skelImp->dimY());
    for (i = 0; i < skelImp->dimX(); ++i) {
        for (j = 0; j < skelImp->dimY(); ++j) {
            bool is_skel_point = skelImp->value(i,j);
            skelLarger->set(i, j, is_skel_point ? 255 : 0);
        }
    } 
    analyze_cca(skelLarger);
    //post_process(skelCurr, imDupeCurr);
    skelLarger = removeDoubleSkel(skelLarger);
    for (i = 0; i < skelLarger->dimX(); ++i) {
        for (j = 0; j < skelLarger->dimY(); ++j) {
            if (skelLarger->value(i, j) > 0) {
                if(ExtendedLayer->value(i,j) == 0)//if dt=0
                    skelLarger->set(i,j,0);
            }
        }
    }
   //if(level==119) skelLarger->writePGM("h.pgm");
    //clipping
    FIELD<float> *clipped = new FIELD<float>(im->dimX(), im->dimY());
    FIELD<float> *clippedDT = new FIELD<float>(im->dimX(), im->dimY());
    for (i = 0; i < im->dimX(); ++i) {
        for (j = 0; j < im->dimY(); ++j) {
            clipped->set(i,j,skelLarger->value(i+leftExtension, j+topExtension));
            clippedDT->set(i,j,ExtendedLayer->value(i+leftExtension, j+topExtension));
        }
    }
  /*   
    stringstream skel;
    skel<<"skel"<<time1<<"-"<<level<<".pgm";
    skelLarger->NewwritePGM(skel.str().c_str());
    skel<<"-clip"<<".pgm";
    clipped->NewwritePGM(skel.str().c_str());
    */  
    //if(time1==3&&level==197) clipped->writePGM("skel197.pgm");
    int seq = 0;
    ///////segment and store into the BranchSets;
    for (int y = 0; y < clipped->dimY(); ++y) {
        for (int x = 0; x < clipped->dimX(); ++x) {
            if (clipped->value(x, y) > 0) {
                vector<Vector3<float>> Branch;
                //if(imDupeCurr->value(x,y) == 0) cout<<"! ";
                tracePath(x, y, clipped, clippedDT, Branch, seq);//get the BranchSet and connection
                seq++;
            }
        }
    }
    
            ////fit with spline///
    //cout<<"BranchSet.size()--"<<BranchSet.size()<<endl;
    int CPnum = 0;
    if(BranchSet.size()>0){
        BSplineCurveFitterWindow3 spline;
        if (WriteToFile>0) spline.SplineFit2(BranchSet, hausdorff, diagonal, level, connection, WriteToFile);
        else CPnum = spline.SplineFit(BranchSet, hausdorff, diagonal, level, connection);
        BranchSet.clear();//important.
        connection.clear();
    }

    delete skelLarger;
    delete clipped; 
    delete clippedDT; 
    return CPnum;
}

/*sign=0: extend top and bottom; sign=1: extend left and right. */
int ComputeExtendLength(int boundaryPos, FIELD<float> * im, int sign){
    int nx = im->dimX();
    int ny = im->dimY();
    float maxl = 0, l;
    int i,j,start,end,Extension;
    if(sign)
    {
        i = boundaryPos;
        for (j = 0; j < ny; ++j) {// get the maxl for this border.
            if(im->value(i,j) == 0)//start
            {
                start = j;

                while(j<ny && im->value(i,j) == 0)
                {
                    j++;
                }//end
                end = j;

                l = end-start;
                if(maxl<l) maxl = l;
                //cout<<"maxl: "<<maxl<<endl;
            }
        }
        Extension = (maxl==ny) ? 0 : ceil(maxl/2)+1;
    }
    else{
        j = boundaryPos;
        for (i = 0; i < nx; ++i) {// get the maxl for this border.
            if(im->value(i,j) == 0)//start
            {
                start = i;

                while(i<nx && im->value(i,j) == 0)
                {
                    i++;
                }//end
                end = i;

                l = end-start;
                if(maxl<l) maxl = l;
                //cout<<"maxl: "<<maxl<<endl;
            }
        }
        Extension = (maxl==nx) ? 0 : ceil(maxl/2)+1;
    }
    
    return Extension;
}

int Image::Extension(int level, FIELD<float> *imDupe, int signal)
{
    /*if(signal>0){
        stringstream ss;
        ss<<"level"<<time1<<"-"<<level<<".pgm";
        imDupe->writePGM(ss.str().c_str());
    }*/
    
    int nx = imDupe->dimX();
    int ny = imDupe->dimY();
    int i,j,i0,j0,l0,r0,dx,dy,r, centerX, centerY;
    
    //compute extension.
    int topExtension = ComputeExtendLength(0, imDupe, 0);
    int bottomExtension = ComputeExtendLength(ny-1, imDupe, 0);
    int leftExtension = ComputeExtendLength(0, imDupe, 1);
    int rightExtension = ComputeExtendLength(nx-1, imDupe, 1);
    //cout<<topExtension<<" "<<bottomExtension<<" "<<leftExtension<<" "<<rightExtension<<endl;

//get the extended version
    int extentedW = nx + leftExtension + rightExtension;
    int extentedH = ny + topExtension + bottomExtension;

    FIELD<float> *ExtendedLayer = new FIELD<float>(extentedW, extentedH);
    for (i = 0; i < extentedW; ++i) {
        for (j = 0; j < extentedH; ++j) {
            ExtendedLayer->set(i, j, 255);//non-object
            if(i > leftExtension-1 && i < leftExtension+nx && j > topExtension-1 && j < topExtension+ny){
                if(imDupe->value(i-leftExtension, j-topExtension) == 0)
                    ExtendedLayer->set(i, j, 0);     
            }
        }
    }
   //ExtendedLayer->NewwritePGM("ExtendedLayer.pgm");
//draw top semi-circle
    int x,y;
    int startX, startY;
    int sx,sy, ex,ey;
    vector<coord3D_t> smallBoundary;

    i = leftExtension;
    j = topExtension;
    //write into the smallBoundary vector.
if(topExtension){//start from the left top corner.
   
    while(ExtendedLayer->value(i,j) == 0) //find a break point.
        i++;
    
    //then start from (i+1,j)
    startX = i+1; startY = j; y=j;
    //cout<<"level "<<level<<"  "<<leftExtension<<" ++ "<<topExtension<<"   "<<rightExtension<<" ++ "<<bottomExtension<<endl;
    for (x = startX; x < leftExtension+nx; ++x) 
        smallBoundary.push_back(coord3D_t(x,y,ExtendedLayer->value(x,y))); 

    x = leftExtension+nx-1;
    if(rightExtension)
    {
        for(y = startY; y < topExtension+ny; ++y)
            smallBoundary.push_back(coord3D_t(x,y,ExtendedLayer->value(x,y)));
    }
    else smallBoundary.push_back(coord3D_t(x,y+1,1));//0-object. 1-break;
    
    y = topExtension+ny-1;
    if(bottomExtension)
    {
        for(x = leftExtension+nx-1; x > leftExtension-1; --x)
            smallBoundary.push_back(coord3D_t(x,y,ExtendedLayer->value(x,y)));
    }
    else smallBoundary.push_back(coord3D_t(x-1,y,1));//0-object. 1-break;

    x = leftExtension;
    if(leftExtension)
    {
        for(y = topExtension+ny-1; y > topExtension-1; --y)
            smallBoundary.push_back(coord3D_t(x,y,ExtendedLayer->value(x,y)));
    }
    else smallBoundary.push_back(coord3D_t(x,y-1,1));//0-object. 1-break;

    y = topExtension;
    for (x = leftExtension; x < startX; ++x) 
        smallBoundary.push_back(coord3D_t(x,y,ExtendedLayer->value(x,y)));
    
}
else{
    smallBoundary.push_back(coord3D_t(i,j,1));//0-object. 1-break;
    x = leftExtension + nx - 1;
    y = topExtension + ny - 1;

    x = leftExtension+nx-1;
    if(rightExtension)
    {
        for(y = topExtension; y < topExtension+ny; ++y)
            smallBoundary.push_back(coord3D_t(x,y,ExtendedLayer->value(x,y)));
    }
    else smallBoundary.push_back(coord3D_t(x,y,1));//0-object. 1-break;

    y = topExtension+ny-1;
    if(bottomExtension)
    {
        for(x = leftExtension+nx-1; x > leftExtension-1; --x)
            smallBoundary.push_back(coord3D_t(x,y,ExtendedLayer->value(x,y)));
    }
    else smallBoundary.push_back(coord3D_t(x-1,y,1));//0-object. 1-break;
    
    x = leftExtension;
    if(leftExtension)
    {
        for(y = topExtension+ny-1; y > topExtension-1; --y)
            smallBoundary.push_back(coord3D_t(x,y,ExtendedLayer->value(x,y)));
        
        smallBoundary.push_back(coord3D_t(x+1,y,1));//make sure the last element is non-object.
    }
    else smallBoundary.push_back(coord3D_t(x,y-1,1));//0-object. 1-break;

}
    //if(level==163) cout<<"smallBoundary.size() "<<smallBoundary.size()<<endl;
vector<coord3D_t>::iterator it;
for(it = smallBoundary.begin(); it!=smallBoundary.end();it++){
    //if(level==163) cout<<"("<<(*it).first<<", "<<(*it).second<<")  ";
    if((*it).third == 0)//start
    {
        sx = (*it).first;
        sy = (*it).second;

        while((*it).third == 0) //pls make sure the last element in the vector is non-0.
            it++; //end
        //cout<<" "<<bool(it!=smallBoundary.end());
        ex = (*it).first;
        ey = (*it).second;
//draw semi-circle.
        centerX = (sx+ex)/2;
        centerY = (sy+ey)/2;
        r = sqrt((centerX-ex)*(centerX-ex)+(centerY-ey)*(centerY-ey)); 
       
        for (i = 0; i < extentedW; ++i) 
            for (j = 0; j < extentedH; ++j)
            {
                if(!(j > topExtension-1 && j < ny+topExtension && i > leftExtension-1 && i < nx+leftExtension))
                {//only draw in the extended region.
                    dx = centerX - i;
                    dy = centerY - j;
                    
                    if(((dx*dx + dy*dy) < (r*r+1)) && ((ex-centerX)*(j-centerY) < (ey-centerY)*(i-centerX)))//Use cross product
                        ExtendedLayer->set(i, j, 0);
                }
            }
    }
    
}
    
   /*  stringstream ss;
    ss<<"Exendedlevel"<<time1<<"-"<<level<<".pgm";
    ExtendedLayer->writePGM(ss.str().c_str());
    */
    int CPnum = GetSkelandCPs(level, ExtendedLayer, topExtension, leftExtension, signal);
    return CPnum;
    //delete ExtendedLayer;
}


/*
* Calculate the skeleton for each layer.
*/
void Image::computeSkeletons() {
    //Outfile.open("time.txt",ios_base::app);
    time1++;
    FIELD<float> *imDupeBack = 0;
    FIELD<float> *imDupeFore = 0;
    FIELD<float> *imDupe;
    FIELD<float> *skelCurr = 0;
    //int seq = 0;
    initialize_skeletonization(im);// Does nothing with CPU skeletons but sets up CUDA skeletonization
   
    diagonal  = sqrt((float)(im->dimX() * im->dimX() + im->dimY() * im->dimY()));
    bool firstTime = true;

    PRINT(MSG_NORMAL, "Computing the skeleton for all layers...\n");
   
    for (int i = 0; i < 256; ++i) {
        //PRINT(MSG_NORMAL, "Layer: %d\n", i);
        if (importance[i] > 0) {
            //cout<<" i "<<i;
            if (firstTime)//Do nothing.
                firstTime = false; 
            else 
            { 
                // Threshold the image
                imDupeBack = im->dupe();
                
                imDupeBack->threshold(i);

                //imDupeBack = removeIslands2(imDupeBack);//little benefit.

                //print every layer.
                /*  stringstream ss;
                ss<<"level"<<time1<<"-"<<i<<".pgm";
                imDupeBack->writePGM(ss.str().c_str());
              */  
                //initialize_skeletonization(im);// Does nothing with CPU skeletons but sets up CUDA skeletonization
   
                if(!adaptive){
                    if(REMOVE_Y) Extension(i, imDupeBack, 2);
                    else CalculateCPnum(i,imDupeBack,2);
                }
                else
                {
                    if(REMOVE_Y){
                        int background = Extension(i, imDupeBack, 0); 

                        imDupeFore = im->dupe();
                        imDupeFore->threshold_(i); //foreground <-> background
                
                        int foreground = Extension(i,imDupeFore,0);
                        cout<<"i "<<i<<" background: "<<background<<" foreground: "<<foreground<<endl;
                        if(foreground==0){
                            clear_color = i; //There are no skeletons produced for this layer, so this layer will be the clear_color layer.
                            //cout<<"clear_color turns to: "<<clear_color<<endl;
                        } 
                        else{
                            int sign;

                            imDupe = im->dupe();
                            if(background > foreground)
                                {imDupe->threshold_(i); sign = 1;}
                            else
                                {imDupe->threshold(i); sign = 2;}//no clip

                            Extension(i, imDupe, sign); 
                        }

                    }
                    else{
                        int background = CalculateCPnum(i,imDupeBack,0);

                        imDupeFore = im->dupe();
                        imDupeFore->threshold_(i); //foreground <-> background
                
                        int foreground = CalculateCPnum(i,imDupeFore,0);
                        //cout<<"i "<<i<<" background: "<<background<<" foreground: "<<foreground<<endl;
                        if(foreground==0){
                            clear_color = i; //There are no skeletons produced for this layer, so this layer will be the clear_color layer.
                            //cout<<"clear_color turns to: "<<clear_color<<endl;
                        } 
                        else{
                            int sign;

                            imDupe = im->dupe();
                            if(background > foreground)
                                {imDupe->threshold_(i); sign = 1;}
                            else
                                {imDupe->threshold(i); sign = 2;}//no clip
 
                            CalculateCPnum(i,imDupe,sign);//orig method.
                        }

                    }
                }
                    
            }
        }
    }

/*if(time1>2){
    Outfile << "Computing skeletons took = " << SkelTime << " (ms)" << std::endl;
    Outfile << "Fitting spline took = " << SplineTime << " (ms)" << std::endl;}
   Outfile.close();
*/
    ofstream OutFile1;
    OutFile1.open("controlPoint.txt",ios_base::app);
    OutFile1<<255<<endl; // the final end tag for one channel.
    
    OutFile1.close(); 
    PRINT(MSG_NORMAL, "\nSkeletonization Done!\n");

    //delete skelImp;
}

