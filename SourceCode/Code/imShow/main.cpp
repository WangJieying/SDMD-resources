#include <fstream>
#include <cmath>
#include <sys/stat.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <unistd.h>
#include <tuple>
#include "include/main.hpp"
#include "include/io.hpp"
#include "include/messages.h"
#include "include/glslProgram.h"
#include "include/framebuffer.hpp"
#include "include/DataManager.hpp"
#include "include/ImageDecoder.hpp"
#include "lodepng/lodepng.h"
#include "include/image.h"
//#include <chrono>
#include "fileio/fileio.hpp"
//wang.

#include "SplineGenerate/BSplineCurveFitter/BSplineCurveFitterWindow3.h"
#include <sstream>
#include <string.h>
using namespace std;

#define MIN(a,b) (a) < (b) ? (a):(b)
#define MAX(a,b) (a) > (b) ? (a):(b)
#define ARRAY_SIZE(arr) sizeof(arr)/sizeof(arr[0])


SHADER_TYPE SHADER = NORMAL;
//SHADER_TYPE SHADER = LOCAL_ALPHA;
// Possible values: GLOBAL_ALPHA, LOCAL_ALPHA, NORMAL;
/* Global alpha uses a distance transform on the entire layer.
 * Local alpha uses a distance for each disk (TESTING)
 * Normal does not perform additional interpolation.
 */
   
/* Output level: MSG_QUIET, MSG_ERROR, MSG_NORMAL, MSG_VERBOSE */
char * i; bool SaveMore = 0; 
int SuperResolution = 1;
vector<int> Invert_, Invert_g, Invert_b;//wang
int MSG_LEVEL = MSG_VERBOSE;
int WWIDTH = 0, WHEIGHT = 0;

unsigned char* new_pixel_data = 0;
DataManager *dm, *dm_g, *dm_b;
bool is_color_image = false;
COLORSPACE c_space = COLORSPACE::NONE;
GLSLProgram *sh_render;
//bool interpolate = true;
bool interpolate = false;
int display1;
using namespace std;
int firstLayerNum; int time1 = 0;//wang
float extend;
//float TotalTime = 0;
//ofstream Outfile1;
char *outFile = 0; /* Press 's' to make a screenshot to location specified by outFile. */

void saveOutput() {
    unsigned char *sdata = (unsigned char *) malloc(WWIDTH * WHEIGHT * 4);
    glReadPixels(0, 0, WWIDTH, WHEIGHT, GL_RGB, GL_UNSIGNED_BYTE, sdata);
    Texture2D *t = new Texture2D(WWIDTH, WHEIGHT, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
    t->setData(sdata);
    t->saveAsPNG(outFile);
    free(sdata);
    delete t;
}

void initDataManager(const char *file) {
    ImageDecoder id;
    if(id.Yremoval) extend = 1.5;
    else extend = 1.0;
    c_space = id.load(file);
    is_color_image = (c_space != COLORSPACE::NONE && c_space != COLORSPACE::GRAY);
    PRINT(MSG_NORMAL, "Is color image? %s\n", is_color_image ? "Yes" : "No");
    
    Invert_.assign(id.Invert.begin(), id.Invert.end());///
    if(is_color_image){
        Invert_g.assign(id.g_Invert.begin(), id.g_Invert.end());///
        Invert_b.assign(id.b_Invert.begin(), id.b_Invert.end());///
        //cout<<"Invert_ "<<Invert_.size()<<" "<<Invert_g.size()<<" "<<Invert_b.size()<<endl;
    }

    BSplineCurveFitterWindow3 spline;
    spline.SplineGenerate(SuperResolution);
     
    if(is_color_image)
        id.loadSample(SuperResolution, 3);
    else id.loadSample(SuperResolution, 1);
    PRINT(MSG_NORMAL, "Load sample!\n");
    
    
    dm = new DataManager();
    dm->setWidth(id.width * SuperResolution);
    dm->setHeight(id.height * SuperResolution);
    dm->initCUDA();
    dm->setData(id.getImage());
    dm->set_clear_color(id.Rfirst);
    dm->initOUT();
    dm->set_gray_levels(id.get_gray_levels());
    //dm->setFirstLayer(id.Rfirst);//wang

    if (is_color_image) {
        dm_g = new DataManager();
        dm_g->setWidth(id.width * SuperResolution);
        dm_g->setHeight(id.height * SuperResolution);
        
        dm_g->setData(id.getGChannel());
        dm_g->set_clear_color(id.Gfirst);
        dm_g->initOUT();
        dm_g->set_gray_levels(id.get_g_levels());
        
        dm_b = new DataManager();
        dm_b->setWidth(id.width * SuperResolution);
        dm_b->setHeight(id.height * SuperResolution);
        
        dm_b->setData(id.getBChannel());
        dm_b->set_clear_color(id.Bfirst);
        dm_b->initOUT();
        dm_b->set_gray_levels(id.get_b_levels());
        
    }
   /**/
   
}

void initShader() {
    string runifs[] = {"alpha",
                       "layer"
                      };
    sh_render = new GLSLProgram("glsl/render.vert", "glsl/render.frag");
    sh_render->compileAttachLink();
    sh_render->bindUniforms(ARRAY_SIZE(runifs), runifs);
}

void draw_image(DataManager* a_dm) {
    vector<int> gray_levels = a_dm->get_gray_levels();
    //cout<<" gray_levels.empty() "<<gray_levels.empty()<<endl;
   
    if(!gray_levels.empty())
    {
        int prev_inty = 0;
        bool last_layer = false;
        vector<int> Invert_current;
        int InvertNum = 0;
        int Prev_invert = 0;
        time1 ++;

        int max_level = *std::max_element(gray_levels.begin(), gray_levels.begin() + gray_levels.size());
   
        if (time1 == 1) { Invert_current.assign(Invert_.begin(), Invert_.end());}
        else if (time1 == 2) {Invert_current.assign(Invert_g.begin(), Invert_g.end());}
            else { Invert_current.assign(Invert_b.begin(), Invert_b.end());}

        for (int inty : gray_levels) {

            if (a_dm->Yremoval) a_dm->ExtendSkel(inty, time1);

            if(interpolate){
                if(inty == max_level) last_layer = true;
                a_dm->get_interp_layer(inty, prev_inty, SuperResolution, last_layer, time1, Prev_invert, Invert_current.at(InvertNum));
          
            }
            else
                a_dm->getAlphaMapOfLayer1(inty, prev_inty, time1, Invert_current.at(InvertNum));
        
            prev_inty = inty;
            Prev_invert = Invert_current.at(InvertNum);
            InvertNum ++;
        }
       
   }

}

// Convert some colorspace back to RGB
tuple<unsigned char, unsigned char, unsigned char> convert_color_space(unsigned char x, unsigned char y, unsigned char z) {
    int r, g, b;
    float r_prime, g_prime, b_prime;

    if (c_space == COLORSPACE::YCC) {
        //cout<<"***"<<endl;
        r = min(max(0, round(x                      + 1.402  * (z - 128))), 255);
        g = min(max(0, round(x - 0.3441 * (y - 128) - 0.7141 * (z - 128))), 255);
        b = min(max(0, round(x + 1.772  * (y - 128)                     )), 255);
    } else if (c_space == COLORSPACE::HSV) {
        float h = (x / 255.0) * 360.0;
        float s = y / 255.0;
        float v = z / 255.0;
        // cout << h << " - " << s << " - " << v << endl;
        if (s == 0.0) {
            r_prime = g_prime = b_prime = z;
        } else {
            int hi = static_cast<int>(h / 60.0f);
            double f = h / 60.0f - hi;
            double p = v * (1.0f - s);
            double q = v * (1.0f - f * s);
            double t = v * (1.0f - (1.0f - f) * s);

            switch (hi) {
            case 0: r_prime = v; g_prime = t; b_prime = p; break;
            case 1: r_prime = q; g_prime = v; b_prime = p; break;
            case 2: r_prime = p; g_prime = v; b_prime = t; break;
            case 3: r_prime = p; g_prime = q; b_prime = v; break;
            case 4: r_prime = t; g_prime = p; b_prime = v; break;
            default: r_prime = v; g_prime = p; b_prime = q; break;
            }
        }
        r = round(r_prime * 255.0);
        g = round(g_prime * 255.0);
        b = round(b_prime * 255.0);
    } else {
        // RGB
        r = x, g = y, b = z;
    }
    return make_tuple(r, g, b);
}

void draw_color_image(DataManager* dm_r, DataManager* dm_g, DataManager* dm_b) {
    
    //FIELD<float>* chan1, chan2, chan3;
    draw_image(dm_r);
    dm_r->output->NewwritePGM("R.pgm");

    draw_image(dm_g);
    dm_g->output->NewwritePGM("G.pgm");

    draw_image(dm_b);
    dm_b->output->NewwritePGM("B.pgm");
     
    cout<<WWIDTH<<"!!"<<endl;
/**/
}


void display(void) {

    cout <<" is_color_image?: " << is_color_image << endl;
    if (is_color_image) {
        draw_color_image(dm, dm_g, dm_b);
    } else {
       draw_image(dm);
       dm->output->NewwritePGM("output.pgm");
    }
    //glFlush();  // Finish rendering
   // glutSwapBuffers();
    //dm->output->NewwritePGM("newOut.pgm");
   
    glutDestroyWindow(display1);
}

void reshape(int w, int h) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WWIDTH*extend, WHEIGHT*extend, 0);
    glViewport(0, 0, WWIDTH*extend, WHEIGHT*extend);
    glMatrixMode(GL_MODELVIEW);
    return;
}

void idle(void) {
    usleep(1000);
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 's':
        outFile = const_cast<char*>("output.png");
        saveOutput();
        return;
        break;
    case 'q':
    case 27:
        exit(0);
        break;
    case 32:
        // TODO(maarten): interpolated image caching?
        interpolate = !interpolate;
        break;
    default:
        printf("Unsupported input: %c", key);
        fflush(stdout);
        break;
    }
    glutPostRedisplay();
}

int main(int argc, char *argv[]) {
    //Outfile1.open("time.txt",ios_base::app);
    /* Read meta data and set variables accordingly */
    if (argc>2) //need to save lots of output images.
    {
        SaveMore = 1;
        i = argv[2];
        if (argc>3 && *argv[3]==char('0'))  interpolate = false; 
    }
       
    initDataManager(argv[1]);
    WHEIGHT = dm->getHeight();
    WWIDTH = dm->getWidth();
    PRINT(MSG_VERBOSE, "Image dimensions: %d x %d\n", WWIDTH, WHEIGHT);


    // Initialize GLUT 
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WWIDTH*extend, WHEIGHT*extend);
    display1 = glutCreateWindow(WINDOW_TITLE);

    // Initialize GLEW 
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        // Problem: glewInit failed, something is seriously wrong. 
        PRINT(MSG_ERROR, "Error: %s\n", glewGetErrorString(err));
        exit(-1);
    }
    PRINT(MSG_NORMAL, "GLEW: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    // Set OPENGL states 
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set clear color one below the first layer that has points

    // Set texture parameters 
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);


    glutDisplayFunc(display);
     
   // glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);
   // glutIdleFunc(idle);


    initShader();
    dm->initFBO();
    dm->setAlphaShader(SHADER);
    if (is_color_image) {
        dm_g->initFBO();
        dm_g->setAlphaShader(SHADER);
        dm_b->initFBO();
        dm_b->setAlphaShader(SHADER);
    }

    
    glutMainLoop();
    /**/
   
    return 0;
}