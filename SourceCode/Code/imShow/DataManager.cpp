#include "include/DataManager.hpp"
#include "include/texture3D.hpp"
#include "include/framebuffer.hpp"
#include "include/io.hpp"
#include "include/messages.h"
#include "include/skeleton_cuda.hpp"
#include <math.h>
#define ARRAY_SIZE(arr) sizeof(arr)/sizeof(arr[0])
#define SET_TEXTURE(arr, i, val) do { (arr)[(4 * (i))] = (arr)[(4 * (i) + 1)] = (arr)[(4 * (i) + 2)] = (val); (arr)[(4 * (i) + 3)] = 255.0; } while(0);

typedef std::pair<int, int> coord2D_t;
typedef vector<coord2D_t> coord2D_list_t;
coord3D_t diff = coord3D_t(0, 0, 0);
int index1, OrigBoundaryLine;
int w,h;
FIELD<float>* prev_layer;
FIELD<float>* prev_layer_dt;

/*************** CONSTRUCTOR ***************/
DataManager::DataManager() {
    dim = new int[2];
    fbo = (Framebuffer *) 0;
    
}

DataManager::~DataManager() {
    delete [] dim;
    delete dataLoc;
    delete fbo;
    delete sh_alpha;
    deallocateCudaMem();
}

void DataManager::initCUDA() {
    initialize_skeletonization(dim[0], dim[1]);
 
}
void DataManager::initOUT() {
    w = dim[0];
    h = dim[1];
    output = new FIELD<float>(dim[0], dim[1]);
    prev_layer = new FIELD<float>(dim[0], dim[1]);
    prev_layer_dt = new FIELD<float>(dim[0], dim[1]);
    //cout<<"clear_color "<<clear_color<<endl;
    //if(clear_color > 124 && clear_color< 130) clear_color = 128;
    for (unsigned int x = 0; x < dim[0]; x++) 
        for (unsigned int y = 0; y < dim[1]; y++)
            output->set(x, y, clear_color);
}


FIELD<float>* DataManager::get_texture_data() {

    unsigned char* data = (unsigned char*)(calloc(dim[0] * dim[1], sizeof(unsigned char)));
    //cout<<"here1"<<endl;
    glReadPixels(0, 0, dim[0], dim[1], GL_RED, GL_UNSIGNED_BYTE, data);
    
    FIELD<float>* f = new FIELD<float>(dim[0], dim[1]);
    
    for (int i = 0; i < dim[0] * dim[1]; ++i) {
        
        int x = i % dim[0];
        int y = i / dim[0];
        
        f->set(x, dim[1] - y - 1, data[i]);
        
      
    }
    free(data);
    data = NULL;
    return f;
}

// Computes the DT of the alpha map. It's settable if the DT needs to be computed
// of the foreground (i.e. pixels which are 0) or the background (i.e. pixels that are 1)
FIELD<float>* DataManager::get_dt_of_alpha(FIELD<float>* alpha, bool foreground) {
    auto alpha_dupe = alpha->dupe();
    auto data = alpha_dupe->data();
    // Widen the image range from [0..1] to [0..255]

    for (int i = 0; i < dim[0] * dim[1]; i++) {
        data[i] *= 255;
    }
    
    auto ret = computeCUDADT(alpha_dupe, foreground);
   
    return ret;
}


void DataManager::get_interp_layer(int intensity, int prev_intensity, int superR, bool last_layer, int time1, int prev_invert, int invert) {
    bool BlackBackground = false;
    bool compute_dt_of_foreground = false;
    unsigned int x, y;
    int curr_bound_value;
    FIELD<float>* curr_alpha = getAlphaMapOfLayer(intensity, superR, time1, invert);

    //if(time1==1 && intensity==51) curr_alpha -> writePGM("51.pgm");
    //print each layer
   /* stringstream ss;
    ss<<"output"<<time1<<"-"<<intensity<<".pgm";
    const char* outFile = const_cast<char*>(ss.str().c_str());
    curr_alpha -> writePGM(outFile);//wang.
    */
    FIELD<float>* curr_dt = get_dt_of_alpha(curr_alpha, !compute_dt_of_foreground);
    //if (prev_intensity == 0) curr_dt->NewwritePGM("first2.pgm");
    //FIELD<float>* prev_alpha = nullptr;
    FIELD<float>* prev_alpha = new FIELD<float>(dim[0], dim[1]);
    FIELD<float>* prev_alpha_forDT = new FIELD<float>(dim[0], dim[1]);
    FIELD<float>* prev_dt = nullptr;

    FIELD<float>* inward_dt = nullptr;
    if(last_layer){
        inward_dt = get_dt_of_alpha(curr_alpha, compute_dt_of_foreground);//inward
        //inward_dt->NewwritePGM("inward.pgm");
    }

    if (prev_intensity == 0) {
        prev_intensity = clear_color;
        prev_bound_value = clear_color;
        for (int i = 0; i < dim[0]; ++i) 
            for (int j = 0; j < dim[1]; ++j){
                    if(i==0 || j==0 || i==dim[0]-1 || j==dim[1]-1)    prev_alpha_forDT -> set(i, j, 0); 
                    else prev_alpha_forDT -> set(i, j, 1); 
                    prev_alpha -> set(i, j, 1);
                }
        prev_dt = get_dt_of_alpha(prev_alpha_forDT, compute_dt_of_foreground);
    }
    else{
        prev_alpha = prev_layer;
        //prev_alpha = getAlphaMapOfLayer(prev_intensity, superR, 4, prev_invert);
        prev_dt = prev_layer_dt;
    }
    
    curr_bound_value = (prev_intensity + intensity)/2;
    

    /*/%%%%%%%%%%%%%%% Prune current layer %%%%%%
    for (x = 0; x < dim[0]; ++x) 
        for (y = 0; y < dim[1]; ++y)
        {
            if(curr_alpha->value(x, y) && !prev_alpha->value(x, y))
                curr_alpha->set(x,y,0);
               
        }
    */
    

    prev_layer = curr_alpha->dupe();
    prev_layer_dt = curr_dt->dupe();
        
    //%%%%%%%%%%%%%%% Interpolation %%%%%%

    for (x = 0; x < dim[0]; ++x) 
        for (y = 0; y < dim[1]; ++y)
        {
            if(!curr_alpha->value(x, y) && prev_alpha->value(x, y))
            {// If there are pixels active between boundaries we smoothly interpolate them
                if(BlackBackground){
                    if(time1 == 1 && prev_intensity == clear_color) ;//Do not interpolate the first layer and the second layer.
                    else if(time1 > 1 && prev_intensity>124 && prev_intensity<131)//cope with black background (i.e. Y:0, Cb:128, Cr:128)
                            output->set(x, y, 128);
                        else{
                            float prev_dt_val = prev_dt->value(x, y);
                            float curr_dt_val = curr_dt->value(x, y);

                            float interp_alpha = prev_dt_val / ( prev_dt_val + curr_dt_val);
                            float interp_color = curr_bound_value * interp_alpha + prev_bound_value *  (1 - interp_alpha);
                            
                            output->set(x, y, interp_color);
                        }
                }
                else{
                    float prev_dt_val = prev_dt->value(x, y);
                    float curr_dt_val = curr_dt->value(x, y);

                    float interp_alpha = prev_dt_val / ( prev_dt_val + curr_dt_val);
                    float interp_color = curr_bound_value * interp_alpha + prev_bound_value *  (1 - interp_alpha);
                    
                    output->set(x, y, interp_color);
                }
            }
        }
   
    prev_bound_value = curr_bound_value;
        if(last_layer){
        
        for (x = 0; x < dim[0]; ++x) 
            for (y = 0; y < dim[1]; ++y){
                if(curr_alpha->value(x, y))
                {
                    //if (time1==2) output->set(x, y, 128);//orig.
                    //else{
                        int interp_last_layer_value = prev_bound_value + (inward_dt->value(x, y)/10);
                        int MaxIntensity = (intensity+10) > 255 ? 255 : (intensity+10);
                        output->set(x, y, (interp_last_layer_value > MaxIntensity) ? MaxIntensity : interp_last_layer_value);
                    //}
                }
            }
    }

    delete curr_alpha;
    delete prev_alpha;
    delete curr_dt;
    delete inward_dt;
    delete prev_dt;
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
FIELD<float> * removeDoubleSkel(FIELD<float> *layer, int w, int h) {
    int topL = h*0.25;
    int bottomL = topL + h - 1;
    int leftL = w*0.25;
    int rightL = leftL + w - 1;

    int *k = (int *)calloc(9, sizeof(int));
    
    for (int y = 0; y < layer->dimY(); ++y) {
        for (int x = 0; x < layer->dimX(); ++x) {
            if (layer->value(x, y)) {
                if(y==topL || y==bottomL || x == leftL || x == rightL) ;
                else{
                    int NeiSkelN = 0;
                    k[0] = layer->value(x - 1, y - 1);
                    k[1] = layer->value(x - 1, y);
                    k[2] = layer->value(x - 1, y + 1);
                    k[3] = layer->value(x  , y - 1);
                    k[4] = 0;
                    k[5] = layer->value(x  , y + 1);
                    k[6] = layer->value(x + 1, y - 1);
                    k[7] = layer->value(x + 1, y);
                    k[8] = layer->value(x + 1, y + 1);
                    for(int i = 0; i<9; ++i)  NeiSkelN += (k[i]>0) ? 1 : 0;
                    if (NeiSkelN > 1) {
                        int b = numObjects(k);
                        if (b < 2 ) layer->set(x, y, 0); 
                    }
                }
                
            }
        }
    }
    free(k);
    return layer;
}

void tracePath(int x, int y, FIELD<float> *skel, bool whichSide){
    index1 ++;
    if(index1 > 20) return;
    coord2D_t n;
    coord2D_list_t *neigh;
    int DT_xy = skel->value(x,y);
	skel->set(x, y, 0);
	neigh = neighbours(x, y, skel);
    
	if (neigh->size() == 1)//==1; same branch.
	{

        n = *(neigh->begin());

        if(index1 == 1)//first time.
        {
            if (whichSide) 
                {if(n.first == x) {skel->set(n.first, n.second, 0); index1 = 100;}} //give a large num; 
            else
                {if(n.second == y) {skel->set(n.first, n.second, 0); index1 = 100;}}//give a large num;
        }
        
        diff.first += n.first - x;
        diff.second += n.second - y;
        diff.third += skel->value(n.first, n.second) - DT_xy;

        tracePath(n.first, n.second, skel, whichSide);//same seq;
	}
    else
    {
        if(index1 == 1){
            if (neigh->size() == 0) index1 = 100;//give a large num;
            else{
                while (neigh->size() > 0) {
                n = *(neigh->begin());
                if (whichSide) 
                {
                    if(n.first != x) tracePath(n.first, n.second, skel, whichSide);
                    else skel->set(n.first, n.second, 0);
                }
                else 
                {
                    if(n.second != y) tracePath(n.first, n.second, skel, whichSide);
                    else skel->set(n.first, n.second, 0);
                }
                neigh->erase(neigh->begin());
                }
            }
            
        }
        else return;
    } 	 
}

int Extend(FIELD<float> *skel, int intersectionY, int l){
    int i, j, k;
    
    FIELD<float>* skelDupe = skel->dupe();
    //skelDupe->writePGM("skelDupe.pgm");
    j = OrigBoundaryLine;
    for (i = 0; i < w; ++i)
    {
        if(skelDupe->value(i,j) > 0){//find border skeleton point
            
            index1 = 0;
            diff = coord3D_t(0, 0, 0);
            //cout<<"*****"<<i<<" // "<<j<<" // "<<endl;
            tracePath(i,j,skelDupe,0);//skelDupe will be removed several points.

            if(index1 > 6 && index1 < 100){
            float avgX = diff.first/float(index1-1);
            float avgY = diff.second/float(index1-1);
            float avgDT = diff.third/float(index1-1);
            //cout<<"*****"<<h*0.125<<endl;
            for(k = 1; k < int(h*0.125); ++k)//Determines how long it extends
            {
                int potentialX = int(i-k*avgX); //linear interpolation.
                int potentialY = int(j-k*avgY);
                int potentialDT = int(skel->value(i,j)-k*avgDT);

                if(potentialX == 0 || potentialX == w-1 || potentialY == 0 || potentialY == h-1) break;
            /* Didn't consider the intersection points
                if(intersectionY==0){
                    if(skel->value(potentialX, potentialY) > 0 || skel->value(potentialX, potentialY-1) > 0) //already has a skel there.
                    {
                        //cout<<" potentialX: "<<potentialX<<" potentialY "<<potentialY<<endl;
                        if (intersectionY < potentialY) {intersectionY = potentialY; break;}
                    }  
                }
                else{
                    if(skel->value(potentialX, potentialY) > 0 || skel->value(potentialX, potentialY+1) > 0) //already has a skel there.
                    {
                         if(l==160) cout<<" potentialX: "<<potentialX<<" potentialY "<<potentialY<<endl;

                        if (intersectionY > potentialY) { intersectionY = potentialY; break;} //
                    } 
                }
            */
                skel->set(potentialX, potentialY, potentialDT);//extend 
              
            }   
            }
        }
    }
    return intersectionY;
}

int ExtendLR(FIELD<float> *skel, int intersectionX){
    int i, j, k;
    
    FIELD<float>* skelDupe = skel->dupe();
    //skelDupe->writePGM("skelDupe.pgm");
    i = OrigBoundaryLine;
    for (j = 0; j < h; ++j)
    {
        if(skelDupe->value(i,j) > 0){
            index1 = 0;
            diff = coord3D_t(0, 0, 0);
            
            tracePath(i,j,skelDupe,1);//skelDupe will be removed several points.

            //cout<<" index1: "<<index1<<endl;
            if(index1 > 6 && index1 < 100){
            float avgX = diff.first/float(index1-1);
            float avgY = diff.second/float(index1-1);
            float avgDT = diff.third/float(index1-1);
            //cout<<"*****"<<avgX<<" // "<<avgY<<" // "<<avgDT<<endl;
            //cout<<"*****"<<h*0.125<<endl;
            for(k = 1; k < int(w*0.125); ++k)//Extend how long
            {
                int potentialX = int(i-k*avgX);
                int potentialY = int(j-k*avgY);
                int potentialDT = int(skel->value(i,j)-k*avgDT);

                if(potentialX == 0 || potentialX == w-1 || potentialY == 0 || potentialY == h-1) break;

                /*if(intersectionX==0){
                    if(skel->value(potentialX, potentialY) > 0 || skel->value(potentialX-1, potentialY) > 0) //already has a skel there.
                    {
                        //cout<<" potentialX: "<<potentialX<<" potentialY "<<potentialY<<endl;
                        if (intersectionX < potentialX) {intersectionX = potentialX; break;}
                    }  
                }
                else{
                    if(skel->value(potentialX, potentialY) > 0 || skel->value(potentialX+1, potentialY) > 0) //already has a skel there.
                    {
                        //cout<<" potentialX: "<<potentialX<<" potentialY "<<potentialY<<endl;

                        if (intersectionX > potentialX) { intersectionX = potentialX; break;} //

                    }    

                }*/

                skel->set(potentialX, potentialY, potentialDT);//extend
            }   } 
        }
    }
    return intersectionX;
}


void DataManager::ExtendSkel(int l, int time1){
    unsigned int i, j;
    w = dim[0]*1.5;
    h = dim[1]*1.5;
    int topE = dim[1]*0.25;
    int leftE = dim[0]*0.25;

    FIELD<float>* skel = new FIELD<float>(w, h);
    for (i = 0; i < w; ++i) 
        for (j = 0; j < h; ++j)
            skel->set(i, j, 0);
   
    layer_t *layer = readLayer(l);
    
    int x, y, radius;

    for (j = 0; j < layer->size(); ++j) {
        x = (*layer)[j].first;
        y = (*layer)[j].second;
        radius = (*layer)[j].third;

        skel->set(x, y, radius);// DT map
    }
    FIELD<float>* skelDupe = skel->dupe();
    skelDupe = removeDoubleSkel(skelDupe, dim[0], dim[1]);
    
  /*
        stringstream skelname;
        skelname<<"sskel"<<time1<<"-"<<l<<".pgm";
        const char* skelFile = const_cast<char*>(skelname.str().c_str());
        skelDupe->NewwritePGM(skelFile);
    
        //if(l==90) skel->writePGM("skel90.pgm");
     */

//Extend the top part
    OrigBoundaryLine = topE;
    int intersectionY = Extend(skelDupe, 0, l); //skelDupe has been extended.
    //cout<<"level: "<<l<<" intersectionY: "<<intersectionY<<endl;

     for (i = 0; i < w; ++i)
        for (j = intersectionY; j < OrigBoundaryLine; ++j)
            if(skelDupe->value(i,j)) layer->push_back(coord3D_t(i, j, skelDupe->value(i,j)));
    
//Extend the bottom part 
    OrigBoundaryLine = topE+dim[1]-1;
    
    intersectionY = Extend(skelDupe, h, l); //skelDupe has been extended.
    
     for (i = 0; i < w; ++i)
        for (j = OrigBoundaryLine; j < intersectionY; ++j)
            if(skelDupe->value(i,j)) layer->push_back(coord3D_t(i, j, skelDupe->value(i,j)));
 
//Extend the left part 
    OrigBoundaryLine = leftE;    
    int intersectionX = ExtendLR(skelDupe, 0);
    for (i = intersectionX; i < OrigBoundaryLine; ++i)
        for (j = 0; j < h; ++j)
            if(skelDupe->value(i,j)) layer->push_back(coord3D_t(i, j, skelDupe->value(i,j)));

//Extend the right part 
    OrigBoundaryLine = leftE+dim[0]-1;    
    intersectionX = ExtendLR(skelDupe, w);
    for (i = OrigBoundaryLine; i < intersectionX; ++i)
        for (j = 0; j < h; ++j)
            if(skelDupe->value(i,j)) layer->push_back(coord3D_t(i, j, skelDupe->value(i,j)));
            
    //if(l==156) skelDupe->writePGM("206.pgm");

}

/* This function draws all disks. The alpha will be 1 at the location that should be drawn, otherwise 0. */
void DataManager::getAlphaMapOfLayer1(int l, int prev_l, int time1, int invert) {
    unsigned int i,j;
   
    layer_t *da = readLayer(l);
    int x, y, radius;

    if (fbo == 0) {
        PRINT(MSG_ERROR, "Framebuffer object was not initialized. Creating new FBO (FIX THIS!)\n");
        initFBO();
    }

    /* Draw on a texture. */
    fbo->bind();
    if (SHADER == GLOBAL_ALPHA) {
        glClearColor(1.0, 1.0, 1.0, 1.0);
    }

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    sh_alpha->bind(); //draw circle rather than quads because of this shader.
    glBegin(GL_QUADS);
    for (unsigned int j = 0; j < da->size(); ++j) {
        x = (*da)[j].first;
        y = (*da)[j].second;
        radius = (*da)[j].third;

       
        // Texture coordinates are in range [-1, 1]
        // Texcoord selects the texture coordinate
        // vertex call sets it to that location
       if(radius>0){
           glTexCoord2f(-1.0, -1.0);
            glVertex2f(x - radius, y - radius);
            glTexCoord2f(-1.0, 1.0);
            glVertex2f(x - radius, y + radius+1);
            glTexCoord2f(1.0, 1.0);
            glVertex2f(x + radius+1, y + radius+1);
            glTexCoord2f(1.0, -1.0);
            glVertex2f(x + radius+1, y - radius);
       }
       
    }
    glEnd();
  
    sh_alpha->unbind();
    fbo->unbind();
    
     FIELD<float>* layer = new FIELD<float>(w, h);
        float *data = (float *) malloc(w * h * sizeof (float));
        
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, fbo->texture->tex);

        // Altering range [0..1] -> [0 .. 255] 
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, data);
    

        //FIELD<float> *im = new FIELD<float>(0, 0);
        //im->setAll(dim[0], dim[1], data);
        //im->writePGM("buffer.pgm"); 
        unsigned int y_;
        for (unsigned int x = 0; x < w; ++x) 
            for (unsigned int y = 0; y < h; ++y)
            {
                y_ = h - 1 -y;

                if(invert){ 
                    if(Yremoval){
                        layer->set(x, y_, 1); //ensure that the init value is 1 everywhere.

                        if(*(data + y * w + x))
                            layer->set(x, y_, 0);
                    }
                    else{
                         if(!*(data + y * w + x))
                            output->set(x, y_, l);
                    }
                   
                }
                else{
                    if(Yremoval){
                        layer->set(x, y_, 0); //ensure that the init value is 0 everywhere.

                        if(*(data + y * w + x))
                            layer->set(x, y_, 1);
                    }
                    else{
                        if(*(data + y * dim[0] + x))
                            output->set(x, y_, l);
                    }
                }
                
            }

    if(Yremoval){
        FIELD<float>* curr_layer = new FIELD<float>(dim[0], dim[1]);
        for (i = 0; i < dim[0]; ++i) 
            for (j = 0; j < dim[1]; ++j){
                curr_layer->set(i,j,0);
                if(layer->value(i+(int)(dim[0]*0.25), j+(int)(dim[1]*0.25))){
                    if(prev_l==0 || (prev_l>0 && prev_layer->value(i, j))){ 
                        curr_layer->set(i,j,1);
                        output->set(i, j, l);
                    }
                }
                    
            }
        prev_layer = curr_layer->dupe();
        delete curr_layer; 
    }
      /*   stringstream ss;
        ss<<"curr"<<time1<<"-"<<l<<".pgm";
        const char* outFile = const_cast<char*>(ss.str().c_str());
        curr_layer -> writePGM(outFile);//wang.
   */
        

    delete layer;  
    free(data);   
}

FIELD<float>* DataManager::getAlphaMapOfLayer(int l, int superR, int time1, int invert) {
    unsigned int i,j;
    
    layer_t *da = readLayer(l);
    int x, y, radius;

    if (fbo == 0) {
        PRINT(MSG_ERROR, "Framebuffer object was not initialized. Creating new FBO (FIX THIS!)\n");
        initFBO();
    }

    /* Draw on a texture. */
    fbo->bind();
    if (SHADER == GLOBAL_ALPHA) {
        glClearColor(1.0, 1.0, 1.0, 1.0);
    }

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    sh_alpha->bind(); //draw circle rather than quads because of this shader.
    glBegin(GL_QUADS);
    for (unsigned int j = 0; j < da->size(); ++j) {
        x = (*da)[j].first;
        y = (*da)[j].second;
        radius = (*da)[j].third;

        if(radius>0) 
        {
            
        // Texture coordinates are in range [-1, 1]
        // Texcoord selects the texture coordinate
        // vertex call sets it to that location
       
            glTexCoord2f(-1.0, -1.0);
            glVertex2f(x - radius, y - radius);
            glTexCoord2f(-1.0, 1.0);
            glVertex2f(x - radius, y + radius+1);
            glTexCoord2f(1.0, 1.0);
            glVertex2f(x + radius+1, y + radius+1);
            glTexCoord2f(1.0, -1.0);
            glVertex2f(x + radius+1, y - radius);
        }
    }
    glEnd();


   /**/
   //if (l==109)  skel->writePGM("109.pgm");
    sh_alpha->unbind();
    fbo->unbind();

    FIELD<float>* layer = new FIELD<float>(w, h);
    float *data = (float *) malloc(w * h * sizeof (float));
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fbo->texture->tex);

    // Altering range [0..1] -> [0 .. 255] 
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, data);

    //FIELD<float> *im = new FIELD<float>(0, 0);
    //im->setAll(dim[0], dim[1], data);
    //im->writePGM("buffer.pgm");
    unsigned int y_;
    for (unsigned int x = 0; x < w; ++x) 
        for (unsigned int y = 0; y < h; ++y)
        {
            y_ = h - 1 -y;

            if(invert){
                layer->set(x, y_, 1); //ensure that the init value is 1 everywhere.

                if(*(data + y * w + x))
                    layer->set(x, y_, 0);
            }
            else{
                layer->set(x, y_, 0); //ensure that the init value is 0 everywhere.

                if(*(data + y * w + x))
                    layer->set(x, y_, 1);
            }
            
        }

    free(data);

    if(Yremoval){
        FIELD<float>* clipped = new FIELD<float>(dim[0], dim[1]);
        for (i = 0; i < dim[0]; ++i) 
            for (j = 0; j < dim[1]; ++j)
                clipped->set(i, j, layer->value(i+(int)(dim[0]*0.25), j+(int)(dim[1]*0.25)));
        return clipped;
    }
   
    return layer;            
}

/* Return all disks for a certain intensity */
layer_t *DataManager::readLayer(int l) {
    return (*skelpixel)[l];
}

/* When the lowest intensity is >0, we should set the clear color to i-1, where
* i is the first intensity with disks.
*/
void DataManager::setClearColor() {
    glClearColor(clear_color / 255.0, clear_color / 255.0, clear_color / 255.0, 0);

    PRINT(MSG_VERBOSE, "Clear color set to: %f, %f, %f (layer %d)\n", clear_color / 255.0, clear_color / 255.0, clear_color / 255.0, clear_color);
}

void DataManager::initFBO() {
    float extend;
    if(Yremoval) extend = 1.5;
    else extend = 1.0;
    fbo = new Framebuffer(int(dim[0]*extend), int(dim[1]*extend), GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, true);
}

void DataManager::setAlphaShader(SHADER_TYPE st) {
    SHADER = st;

    if (st == LOCAL_ALPHA) {
        sh_alpha = new GLSLProgram("glsl/alpha.vert", "glsl/alpha.frag");
        sh_alpha->compileAttachLink();
        PRINT(MSG_VERBOSE, "Succesfully compiled shader 1 (local alpha computation)\n");
    } else if (st == GLOBAL_ALPHA) {
        sh_alpha = new GLSLProgram("glsl/nointerpolationinv.vert", "glsl/nointerpolationinv.frag");
        sh_alpha->compileAttachLink();
        PRINT(MSG_VERBOSE, "Succesfully compiled shader 1 (global alpha computation)\n");
    } else if (st == NORMAL) {
        sh_alpha = new GLSLProgram("glsl/nointerpolation.vert", "glsl/nointerpolation.frag");
        sh_alpha->compileAttachLink();
        PRINT(MSG_VERBOSE, "Succesfully compiled shader 1 (No interpolation)\n");
    } else {
        PRINT(MSG_ERROR, "Invalid shader. Did not compile.");
        exit(1);
    }
}