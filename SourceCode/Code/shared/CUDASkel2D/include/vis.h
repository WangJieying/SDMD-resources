#pragma once


//	Display:		Very simple class to hold the visualization params (thus, not part of the skeletonization/FT engine per se)
//
//


#include <stdlib.h>
#include <GL/glut.h>



class	Display {
public:


    Display(int winSize, int texsize, short* FT, unsigned char* skel, float* siteParam, int endpoints, short* nendpoints,
            float* skelDT, float length, float thr,
            int xm, int ym, int xM, int yM, int argc, char** argv);
    void	generateTexture();

    int    imgSize;												//Effective size (pow 2) of texture images to be displayed
    GLuint texture;												//Visualization parameters
    short* FT;
    unsigned char* skel;
    float* skelDT;
    float* siteParam;
    int    winSize;
    float  scale, transX, transY;
    bool   isLeftMouseActive, isRightMouseActive;
    int    oldMouseX, oldMouseY;
    int    show_what;
    float  threshold;
    short* endpoints;
    int    nendpoints;
    bool   tex_interp;
    int    xm, ym, xM, yM;
    const float length;

private:


    static void mouse(int button, int state, int x, int y);
    static void motion(int x, int y);
    static void display();
    static void keyboard(unsigned char k, int, int);

    static Display*
    instance;
};






