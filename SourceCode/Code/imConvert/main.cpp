/* main.cpp */

#include <math.h>
#include "fileio/fileio.hpp"
#include <string>
#include <sys/resource.h>
#include "main.hpp"
#include <omp.h>
#include <vector>
#include "include/Image.hpp"
#include "include/io.hpp"
#include "include/messages.h"
#include "include/image.h"
#include "include/ImageWriter.hpp"
#include "configParser/include/Config.hpp"
#include <chrono>
#include <boost/algorithm/string.hpp>
#include <fstream>
using namespace std;
 

/* All properties that can be set in the config files: */
int MSG_LEVEL = MSG_NORMAL;
string filename_stdstr;
string compress_method;
/* Image space variables */
float islandThreshold = 0;
float layerThreshold = 0;
int DistinguishableInterval = 0;
COLORSPACE c_space;
extern float SKELETON_DT_THRESHOLD;
extern float SKELETON_SALIENCY_THRESHOLD;
extern float SKELETON_ISLAND_THRESHOLD;
extern string OUTPUT_FILE;
extern string COMP_METHOD_NAME;
extern string ENCODING;
extern bool MSIL_SELECTION;
// Overlap pruning
extern bool OVERLAP_PRUNE;
extern float OVERLAP_PRUNE_EPSILON;
// Bundling variables
extern bool BUNDLE;
extern int EPSILON;
extern float ALPHA;
extern int COMP_LEVEL;
/* Tree variables */
extern int MIN_OBJECT_SIZE;
extern int MIN_SUM_RADIUS;
extern int MIN_PATH_LENGTH;
extern float hausdorff;
float Thresholdtime = 2;
//ofstream Outfile;
// Set special colorspace in case of color image
// Defaults to RGB when an color image is encountered, otherwise to gray
COLORSPACE set_color_space(string c_space) {
    if (boost::iequals(c_space, "hsv")) {
        return COLORSPACE::HSV;
    } else if (boost::iequals(c_space, "ycbcr") || boost::iequals(c_space, "yuv")) {
        return COLORSPACE::YCC;
    } else if (boost::iequals(c_space, "rgb")) {
        return COLORSPACE::RGB;
    } else {
        return COLORSPACE::NONE;
    }
}
/* Set all parameters according to the configuration file parsed
 * with the Config object */
void setConfig(Config c) {
    if (!c.exists("filename")) {
        cout << "Please specify a filename" << endl;
        exit(-1);
    }
    filename_stdstr = c.pString("filename");
    if (!c.exists("outputLevel")) {
        MSG_LEVEL = MSG_NORMAL;
    } else {
        switch (c.pString("outputLevel")[0]) {
        case 'q':
            MSG_LEVEL = MSG_NEVER;
            break;
        case 'e':
            MSG_LEVEL = MSG_ERROR;
            break;
        case 'n':
            MSG_LEVEL = MSG_NORMAL;
            break;
        case 'v':
            MSG_LEVEL = MSG_VERBOSE;
            break;
        }
    }
 
    // MSG_LEVEL = MSG_VERBOSE;
    DistinguishableInterval = c.exists("distinguishable_interval") ? c.pInt("distinguishable_interval") : 5;//Wang
    //Thresholdtime = c.exists("Thresholdtime") ? c.pInt("Thresholdtime") : 2;//Wang
    hausdorff = c.exists("hausdorff") ? c.pDouble("hausdorff") : 0.003;//Wang
    layerThreshold = c.exists("num_layers") ? c.pDouble("num_layers") : (c.exists("lThreshold") ? c.pDouble("lThreshold") : 0);
    islandThreshold = c.exists("islandThreshold") ? c.pDouble("islandThreshold") : 0;
    COMP_METHOD_NAME = c.exists("compression_method") ? c.pString("compression_method") : "lzma";
    COMP_LEVEL = c.exists("compression_level") ? c.pInt("compression") : -1;
    ENCODING = c.exists("encoding") ? c.pString("encoding") : "standard";
    MSIL_SELECTION = c.exists("layer_selection") ? c.pString("layer_selection") != "threshold" : false;
    SKELETON_DT_THRESHOLD = c.exists("sdtThreshold") ? c.pDouble("sdtThreshold") : 5;
    SKELETON_SALIENCY_THRESHOLD = c.exists("ssThreshold") ? c.pDouble("ssThreshold") : 1;
    SKELETON_ISLAND_THRESHOLD = c.exists("SkeletonThreshold") ? c.pDouble("SkeletonThreshold") : 10;
 
    MIN_OBJECT_SIZE = c.exists("minObjSize") ? c.pInt("minObjSize") : 5;
    MIN_SUM_RADIUS = c.exists("minSumRadius") ? c.pInt("minSumRadius") : 15;
    MIN_PATH_LENGTH = c.exists("minPathLength") ? c.pInt("minPathLength") : 3;
 
    OVERLAP_PRUNE = c.exists("overlap_pruning") ? c.pBool("overlap_pruning") : false;
    OVERLAP_PRUNE_EPSILON = c.exists("overlap_pruning_epsilon") ? c.pDouble("overlap_pruning_epsilon") : 0.0001;
    BUNDLE = c.exists("bundle") ? c.pBool("bundle") : false;
    ALPHA = c.exists("alpha") ? min(1, max(0, c.pDouble("alpha"))) : 0;
    EPSILON = c.exists("epsilon") ? c.pInt("epsilon") : 0;
    OUTPUT_FILE = c.exists("outputFile") ? c.pString("outputFile") : "default.sir";
    c_space = c.exists("colorspace") ? set_color_space(c.pString("colorspace")) : COLORSPACE::NONE;
    
} 
       
  int execute_skeleton_pipeline(FIELD<float>* im, float thresholdTime) { 
    int clear_color;

    int Island_thresholdTime = (thresholdTime == 1) ? 1 : (thresholdTime + 3);
       
    Image* il = new Image(im, islandThreshold * Island_thresholdTime, layerThreshold/(thresholdTime), DistinguishableInterval);
   
    il->removeIslands();
    
    il->calculateImportance();
    il->removeLayers();
    
    il->computeSkeletons();

    clear_color = il->clear_color;
    
    delete il;
    return clear_color;
}

string colorspace_to_string() {
    switch (c_space) {
    case GRAY:
        return string("Gray");
    case RGB:
        return string("RGB");
    case HSV:
        return string("HSV");
    case YCC:
        return string("YCbCr");
    default:
        return string("NONE! (FIX THIS)");
    }
}

int get_min_elem(FIELD<float>* im) {
    int min_elem = 1e5;
    int nPix = im->dimX() * im->dimY();
    float *c = im->data();
    float *end = im->data() + nPix;
    while (c != end)
        min_elem = min(min_elem, *c++);
    return min_elem;
}

void execute_gray_pipeline(FIELD<float>* im) {
    
    int firstLayer = 0;
    firstLayer = execute_skeleton_pipeline(im, 1);
    PRINT(MSG_NORMAL, "Creating ImageWriter object...\n");
    ImageWriter iw(OUTPUT_FILE.c_str());
    if (c_space == COLORSPACE::NONE)
        c_space = COLORSPACE::GRAY;
    PRINT(MSG_NORMAL, "Using colorspace %s\n", colorspace_to_string().c_str());
    iw.writeCP(im->dimX(), im->dimY(), c_space, firstLayer);
    //cout<<"here"<<endl;
    delete im;
}

void execute_color_pipeline(IMAGE<float>* im) {
    for (float* r = im->r.data(), *g = im->g.data(), *b = im->b.data(), *rend = im->r.data() + im->dimX() * im->dimY(); r < rend; ++r, ++g, ++b) {
        unsigned char r_prime = static_cast<unsigned char>((*r) * 255.0);
        unsigned char g_prime = static_cast<unsigned char>((*g) * 255.0);
        unsigned char b_prime = static_cast<unsigned char>((*b) * 255.0);
        unsigned char Y, Cb, Cr;
        float H, S, V;
        double max_val, min_val, delta;
        switch (c_space) {
        case COLORSPACE::YCC:
        //cout<<"color ycc"<<endl;
            Y  = min(max(0, round( 0.299  * r_prime + 0.587  * g_prime + 0.114  * b_prime      )), 255);
            Cb = min(max(0, round(-0.1687 * r_prime - 0.3313 * g_prime + 0.5    * b_prime + 128)), 255);
            Cr = min(max(0, round( 0.5    * r_prime - 0.4187 * g_prime - 0.0813 * b_prime + 128)), 255);
            r_prime = Y;
            g_prime = Cb;
            b_prime = Cr;
            if(r == im->r.data() + im->dimX()*5 + 5) cout<<"---------Y "<<(int)Y<<" Cb "<<(int)Cb<<" Cr--------- "<<(int)Cr<<endl;
            break;
        case COLORSPACE::HSV:
            min_val = fmin(*r, fmin(*g, *b));
            max_val = fmax(*r, fmax(*g, *b));
            delta = max_val - min_val;
            H = 0.0;
            V = max_val;
            S = max_val > 1e-6 ? delta / max_val : 0.0f;

            if (S > 0.0f) {
                if (*r == max_val)       H = (0.0f + (*g - *b) / delta) * 60.0;
                else if (*g == max_val)  H = (2.0f + (*b - *r) / delta) * 60.0;
                else                H = (4.0f + (*r - *g) / delta) * 60.0;


                if (H < 0.0f)   H += 360.0f;
            }
            H = fmod(H, 360.0f) / 360.0f;
            r_prime = round(H * 255.0);
            g_prime = round(S * 255.0);
            b_prime = round(V * 255.0);
            break;
        default:
            // Do nothing and encode RGB
            break;
        }
        *r = r_prime, *g = g_prime, *b = b_prime;
    }

    FIELD<float> red_channel   = im->r;
    FIELD<float> green_channel = im->g;
    FIELD<float> blue_channel  = im->b;
    red_channel.NewwritePGM("R.pgm");
    green_channel.NewwritePGM("G.pgm");
    blue_channel.NewwritePGM("B.pgm");
    int num_layers_old;
    if (c_space == COLORSPACE::NONE)
        c_space = COLORSPACE::RGB;
    
    int Rfirst = 0;
    int Gfirst = 0;
    int Bfirst = 0;
    Rfirst = execute_skeleton_pipeline(&red_channel, 1);
    if(c_space == COLORSPACE::YCC){
        //SKELETON_SALIENCY_THRESHOLD *= Thresholdtime;
        if(Thresholdtime > 1) SKELETON_SALIENCY_THRESHOLD += 0.7;
        
        Gfirst = execute_skeleton_pipeline(&green_channel, Thresholdtime);
        Bfirst = execute_skeleton_pipeline(&blue_channel, Thresholdtime);
        //Bfirst = execute_skeleton_pipeline(&blue_channel, 1);
       
    }
    else{
        Gfirst = execute_skeleton_pipeline(&green_channel, 1);
        Bfirst = execute_skeleton_pipeline(&blue_channel, 1);
    }
    
    PRINT(MSG_NORMAL, "Creating ImageWriter object...\n");
    ImageWriter iw(OUTPUT_FILE.c_str());
    PRINT(MSG_NORMAL, "Using colorspace %s\n", colorspace_to_string().c_str());
    iw.writeColorCP(im->dimX(), im->dimY(), c_space, Rfirst, Gfirst, Bfirst);
}

int main(int argc, char **argv) {
   
//////clear controlPoint.txt file at the very first time.
    ofstream clear;
    clear.open("controlPoint.txt");
    clear.close();
///////////////
    
    // Increase the stacksize because default is not enough for some reason.
    const rlim_t kStackSize = 128 * 1024 * 1024;   // min stack size = 128 MB
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0) {
        if (rl.rlim_cur < kStackSize) {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0) {
                fprintf(stderr, "setrlimit returned result = %d\n", result);
            } else {
                PRINT(MSG_NORMAL, "Set stack limit to %d\n", kStackSize);
            }
        }
    }

    /* Parse options */
    if (argc != 2) {
        fstream fhelp(". / doc / USAGE", fstream::in);
        cout << fhelp.rdbuf();
        fhelp.close();

        exit(-1);
    }
    Config config = Config(argv[1], NULL);
    setConfig(config);
    if (argc == 3) {
        OUTPUT_FILE = argv[2];
    }
    const char* filename = filename_stdstr.c_str();
    PRINT(MSG_NORMAL, "Reading input file : % s\n", filename);
    // TODO(maarten): Rely on smt better than file type detection or expand the list
    bool is_color_image = (FIELD<float>::fileType((char*)filename) ==
                           FIELD<float>::FILE_TYPE::PPM);
    if (is_color_image) {
        IMAGE<float>* f = IMAGE<float>::read(filename); /* Read scalar field input */
        if (!f) {
            PRINT(MSG_ERROR, "Failed to read file.\n");
            exit(EXIT_FAILURE);
        } 
        PRINT(MSG_NORMAL, "Executing color pipeline\n");
        execute_color_pipeline(f);
    } else {
        PRINT(MSG_NORMAL, "Executing grayscale pipeline\n");
        FIELD<float>* field = FIELD<float>::read(filename);
 
        if (!field) {
            PRINT(MSG_ERROR, "Failed to read file.\n");
            exit(EXIT_FAILURE);
        }
        execute_gray_pipeline(field);
    }
    //Outfile.close();
    PRINT(MSG_NORMAL, "-----------Done with everything!!!!---------\n");
    return 0;
}




