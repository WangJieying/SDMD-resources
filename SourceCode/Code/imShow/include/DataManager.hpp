/*
 * File:   DataManager.hpp
 * Author: Yuri Meiburg
 *
 * Created on May 1, 2011, 12:21 PM
 */

#ifndef DATAMANAGER_HPP
#define DATAMANAGER_HPP
#include <iostream>
#include <fstream>
#include <cstdlib>
#include "framebuffer.hpp"
#include "texture3D.hpp"
#include "glslProgram.h"
#include "ImageDecoder.hpp"
#include "CUDASkel2D/include/field.h"

#include <vector>
using namespace std;

typedef enum SHADER_TYPE_ {
    INVALID = -1,
    NORMAL,
    LOCAL_ALPHA,
    GLOBAL_ALPHA
} SHADER_TYPE;


typedef struct DataArr_ {
    float *data;
    int nEl;
} DataArr;

class DataManager {
private:
    int *dim;
    int clear_color;
    int prev_bound_value;
    vector<int> gray_levels;
    vector<int> Invert_;//wang
    //int FirstLayer;//wang
    Texture3D *dataLoc;
    Framebuffer *fbo;
    GLSLProgram *sh_alpha;
    SHADER_TYPE SHADER;

public:
    DataManager();
    ~DataManager();

    //Texture2D* get_interp_layer(int intensity, int prev_intensity, int prev_invert, int invert,int time1,bool first);
    void get_interp_layer(int intensity, int prev_intensity, int superR, bool last_layer, int time1, int prev_invert, int invert);
    void initFBO();
    void setAlphaShader(SHADER_TYPE st);
    void setData(image_t *im) {   skelpixel = im;    }
    void setWidth (int x) { dim[0] =  x; };
    void setHeight(int y) { dim[1] =  y; };
    //void set_clear_inty(int c) {clear_color = c;}
    int getWidth() {     return dim[0];    };
    int getHeight() {    return dim[1];    };
    void        set_gray_levels(vector<int>& g_l) { gray_levels = g_l;}//;
    //void setFirstLayer (int firstLayer) {FirstLayer = firstLayer;}//wang
    vector<int>& get_gray_levels() { return gray_levels;}//;
    //int getFirstLayer() {return FirstLayer;}//wang
    void setClearColor();
    void set_clear_color(int c) { clear_color = c;}
    int get_clear_color() {return clear_color;}
    FIELD<float>* get_texture_data();
    void set_fbo_data(unsigned char* texdata) {fbo->texture->setData(texdata);};
    void initCUDA();
    void initOUT();

    void getAlphaMapOfLayer1(int l, int prev_l, int time1, int invert);
    void ExtendSkel(int l, int time11);
    FIELD<float>* getAlphaMapOfLayer(int l, int superR, int time1, int invert);
    //Texture2D* getWhiteMap();
    FIELD<float>* output;
    bool Yremoval = false;
   // bool Yremoval = true;
   
    //int prev_layer_change = 0;
private:
    image_t *skelpixel;
    layer_t *readLayer(int l);
    //FIELD<float>* get_alpha_to_field(int intensity);
    FIELD<float>* get_dt_of_alpha(FIELD<float>* alpha, bool background);

};

#endif  /* DATAMANAGER_HPP */

