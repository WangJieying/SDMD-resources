#include "include/vis.h"
#include "include/skelft.h"
#include <iostream>
#include <math.h>


using namespace std;


Display*    Display::instance = 0;



Display::Display(int winSize_, int texsize, short* FT_, unsigned char* skel_, float* sp, int nendpoints_, short* endpoints_,
                 float* skelDT_, float len, float thr, int xm_, int ym_, int xM_, int yM_, int argc, char** argv):
    imgSize(texsize), FT(FT_), skel(skel_), siteParam(sp),
    winSize(winSize_), scale(1), endpoints(endpoints_), nendpoints(nendpoints_), skelDT(skelDT_),
    transX(0), transY(0), isLeftMouseActive(false), isRightMouseActive(false), oldMouseX(0), oldMouseY(0),
    show_what(0), length(len), threshold(thr), tex_interp(true), xm(xm_), ym(ym_), xM(xM_), yM(yM_) {
    instance = this;

    glutInitWindowSize(winSize, winSize);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);
    glutInit(&argc, argv);
    glutCreateWindow("2D Skeletons and FT");
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);

    glGenTextures(1, &texture);
}



void Display::display() {
    // Initialization
    glViewport(0, 0, (GLsizei) instance->winSize, (GLsizei) instance->winSize);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Setup projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);

    // Setup modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(instance->scale, instance->scale, 1.0);
    glTranslatef(instance->transX, instance->transY, 0.0);

    // Setting up lights
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (instance->tex_interp) ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (instance->tex_interp) ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    // Draw the sample scene
    glBindTexture(GL_TEXTURE_2D, instance->texture);

    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex2f(0.0, 0.0);
    glTexCoord2f(1.0, 0.0); glVertex2f(1.0, 0.0);
    glTexCoord2f(1.0, 1.0); glVertex2f(1.0, 1.0);
    glTexCoord2f(0.0, 1.0); glVertex2f(0.0, 1.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glFinish();


    glutSwapBuffers();
}


void Display::generateTexture() {
    //Encode the FT output into a colored texture
    typedef unsigned char BYTE;

    BYTE* tex = new BYTE[imgSize * imgSize * 3];                                                // Local buffer to store the GL texture

    float range = 100;

    for (int i = 0; i < imgSize; ++i)                                                           // Generate visualization texture
        for (int j = 0; j < imgSize; ++j) {
            int id = j * imgSize + i;                                                           // Position of (i,j) in the various 1D arrays
            float p = siteParam[id];
            BYTE r, g, b; r = g = b = 0;
            if (p)                                                                              // Boundary (site): mark as red
            { r = 255; g = b = 0; }
            else {                                                                              // Regular (off-boundary): show data
                if (show_what == 0) {                                                             // Show either simplified skeleton or FT
                    cout << 0 << endl;
                    float value = 0;
                    if (FT) {
                        int ox = FT[id * 2];                                                           // Coords of the closest site to crt pixel
                        int oy = FT[id * 2 + 1];
                        int vid = oy * imgSize + ox;                                                   // Idx in image arrays of site with coords (ox,oy)
                        value = siteParam[vid] / length;                                               // FT value (color-coded) at current pixel (normalized 0..1 i.e. b/w)
                    }
                    r = g = b = BYTE(255 * value);
                } else if (show_what == 1) {
                    cout << 1  << endl;

                    // Show skeleton branches
                    if (skel[id]) r = g = b = 255;                                                // This is a branch non-endpoint
                } else { //show_what==2
                    cout << 2 << endl;

                    float value = (skelDT) ? skelDT[id] : 0;
                    r = g = b = BYTE(255 * pow(1 - value, 0.2));
                }
            }
            tex[id * 3 + 0] = r;
            tex[id * 3 + 1] = g;
            tex[id * 3 + 2] = b;
        }

    if (show_what == 1)
        for (int i = 0; i < nendpoints; i++) {
            short x = endpoints[2 * i], y = endpoints[2 * i + 1];
            int  id = y * imgSize + x;
            tex[id * 3 + 0] = 0;
            tex[id * 3 + 1] = 255;
            tex[id * 3 + 2] = 0;
        }

    // Create the texture; the texture-id is already allocated
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgSize, imgSize, 0, GL_RGB, GL_UNSIGNED_BYTE, tex);

    delete[] tex;
}







void Display::mouse(int button, int state, int x, int y) {
    if (state == GLUT_UP)
        switch (button) {
        case GLUT_LEFT_BUTTON:
            instance->isLeftMouseActive = false;
            break;
        case GLUT_RIGHT_BUTTON:
            instance->isRightMouseActive = false;
            break;
        }

    if (state == GLUT_DOWN) {
        instance->oldMouseX = x;
        instance->oldMouseY = y;

        switch (button) {
        case GLUT_LEFT_BUTTON:
            instance->isLeftMouseActive = true;
            break;
        case GLUT_RIGHT_BUTTON:
            instance->isRightMouseActive = true;
            break;
        }
    }
}




void Display::motion(int x, int y) {
    if (instance->isLeftMouseActive) {
        instance->transX += 2.0 * double(x - instance->oldMouseX) / instance->scale / instance->imgSize;
        instance->transY -= 2.0 * double(y - instance->oldMouseY) / instance->scale / instance->imgSize;
        glutPostRedisplay();
    } else if (instance->isRightMouseActive) {
        instance->scale -= (y - instance->oldMouseY) * instance->scale / 400.0;
        glutPostRedisplay();
    }

    instance->oldMouseX = x; instance->oldMouseY = y;
}



void Display::keyboard(unsigned char k, int, int) {
    int imgSize = instance->imgSize;
    int xm = instance->xm, ym = instance->ym, xM = instance->xM, yM = instance->yM;

    switch (k) {
    case '.':  instance->scale *= 0.9; break;
    case ',':  instance->scale *= 1.1; break;
    case '-':  instance->threshold--; if (instance->threshold < 1) instance->threshold = 1;
        skelft2DSkeleton(instance->skel, instance->length, instance->threshold, xm, ym, xM, yM);
        instance->nendpoints = 1000;
        skelft2DTopology(0, &instance->nendpoints, instance->endpoints, xm, ym, xM, yM);
        instance->generateTexture(); break;
    case '=':  instance->threshold++; if (instance->threshold > instance->length) instance->threshold = instance->length;
        skelft2DSkeleton(instance->skel, instance->length, instance->threshold, xm, ym, xM, yM);
        instance->nendpoints = 1000;
        skelft2DTopology(0, &instance->nendpoints, instance->endpoints, xm, ym, xM, yM);
        instance->generateTexture(); break;
    case ' ':  instance->show_what++; if (instance->show_what > 2) instance->show_what = 0; instance->generateTexture(); break;
    case 't':  instance->tex_interp = !instance->tex_interp; break;
    case 27:   exit(0);
    }

    glutPostRedisplay();
}





