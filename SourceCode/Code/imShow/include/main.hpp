#ifndef MAIN_GUARD
#define MAIN_GUARD

#define WINDOW_HEIGHT 800
#define WINDOW_WIDTH 800
#define WINDOW_TITLE "Image compression using skeletons - Yuri Meiburg (1634518) - Rijksuniversiteit Groningen - 2010/2011"

#include <GL/freeglut.h>

void glDrawString(float x, float y, const char *s, int large);
void display(void);
void reshape(int w, int h);
void idle(void);
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
int  main (int argc, char **argv);
#endif
