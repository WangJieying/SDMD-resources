#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stack>
#include <vector>
#include <iostream>
#include "include/genrl.h"

using namespace std;


const float ALIVE = -10, NARROW_BAND = ALIVE + 1, FAR_AWAY = ALIVE + 2; //Classification values for pixels in an image. The values are arbitrary.

unsigned long nextPowerOfTwo(unsigned long v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}


int skelft2DSize(int nx, int ny) {
    return max(nextPowerOfTwo(nx), nextPowerOfTwo(ny));
}
/*
void classify(vector<Coord>& s, unsigned char* f, int xm, int ym, int xM, int yM, int size, short iso, bool thr_upper, float* siteParam, bool scan_bot_to_top) {
    for (int i = 0; i < size * size; ++i) siteParam[i] = ALIVE;

    for (int j = ym; j < yM; ++j)
        for (int i = xm; i < xM; ++i) {
            short fptr = f[i + j * size];
            if (((thr_upper && fptr >= iso) || (!thr_upper && fptr <= iso))) 
                siteParam[i + size * j] = FAR_AWAY;
            
        }
        
    int js, je, jd;
    if (scan_bot_to_top) {
        js = ym; je = yM; jd = 1;
    } else {
        js = yM - 1; 
        je = ym - 1;
        jd = -1;
    }
    
    for (int j = js; j != je; j += jd)
        for (int i = xm; i < xM; ++i) {
            float& cv = siteParam[i + size * j];
            if (cv == FAR_AWAY)
            {
                if (i==0 || j==0 || i==xM-1 || j==yM-1) //wang, If it lays on image's edge. then it's BAND.
                {
                    cv = NARROW_BAND;
                    s.push_back(Coord(i, j));
                }
                else if (siteParam[i - 1 + size * j] == ALIVE || siteParam[i + 1 + size * j] == ALIVE || siteParam[i + size * (j - 1)] == ALIVE || siteParam[i + size * (j + 1)] == ALIVE) 
                {
                    if (siteParam[i - 1 + size * j] == ALIVE)  {siteParam[i - 1 + size * j] = NARROW_BAND; s.push_back(Coord(i-1, j));}
                    if (siteParam[i + 1 + size * j] == ALIVE)  {siteParam[i + 1 + size * j] = NARROW_BAND; s.push_back(Coord(i+1, j));}
                    if (siteParam[i + size * (j - 1)] == ALIVE)  {siteParam[i + size * (j - 1)] = NARROW_BAND; s.push_back(Coord(i, (j - 1)));}
                    if (siteParam[i + size * (j + 1)] == ALIVE)  {siteParam[i + size * (j + 1)] = NARROW_BAND; s.push_back(Coord(i, (j + 1)));}
                    
                }
            }

        }
}
*/

//Classify 'f' into inside, outside, and boundary pixels according to isovalue 'iso'
//Inside pixels are those with value > iso (if thr_upper==true), else those with value < iso.
void classify(vector<Coord>& s, unsigned char* f, int xm, int ym, int xM, int yM, int size, short iso, bool thr_upper, float* siteParam, bool scan_bot_to_top) {
    for (int i = 0; i < size * size; ++i) siteParam[i] = ALIVE;

    for (int j = ym; j < yM; ++j)
        for (int i = xm; i < xM; ++i) {
            short fptr = f[i + j * size];
            if (((thr_upper && fptr >= iso) || (!thr_upper && fptr <= iso))) siteParam[i + size * j] = FAR_AWAY;
        }
        
    int js, je, jd;
    if (scan_bot_to_top) {
        js = ym; je = yM; jd = 1;
    } else {
        js = yM - 1; 
        je = ym - 1;
        jd = -1;
    }
    
    for (int j = js; j != je; j += jd)
        for (int i = xm; i < xM; ++i) {
            float& cv = siteParam[i + size * j];
            if (cv == FAR_AWAY)
            {
                if (i==0 || j==0 || i==xM-1 || j==yM-1) //wang, If it lays on image's edge. then it's BAND.
                {
                    cv = NARROW_BAND;
                    s.push_back(Coord(i, j));
                }
                else if (siteParam[i - 1 + size * j] == ALIVE || siteParam[i + 1 + size * j] == ALIVE || siteParam[i + size * (j - 1)] == ALIVE || siteParam[i + size * (j + 1)] == ALIVE) 
                {
                    cv = NARROW_BAND;
                    s.push_back(Coord(i, j));
                }
            }
        }
}






// Encode input image (classified as inside/outside/boundary into a boundary-length parameterization image, needed for skeleton computations.
//
float encodeBoundary(vector<Coord>& s, int xm, int ym, int xM, int yM, float* siteParam, int size) {
    const float LARGE_BOUNDARY_VAL = 1000;             //Value that the boundary-param will jump with between disjoint, self-connected, boundary pixel chains

    float length = -LARGE_BOUNDARY_VAL + 1;            //Compute the boundary length

    while (s.size()) {                              //Process all narrowband points, add their arc-length-parameterization in siteParam
        Coord pt = s[s.size() - 1]; s.pop_back();          //Pick narrowband-point, test if already numbered
        if (siteParam[pt.i + size * pt.j] > 0) continue;

        int ci, cj; float cc = 0;                       //Point pt not numbered
        for (int dj = -1; dj < 2; ++dj)                 //Find min-numbered-neighbour of pt
            for (int di = -1; di < 2; ++di) {
                int ii = pt.i + di, jj = pt.j + dj;
                if (ii < xm || ii >= xM || jj < ym || jj >= yM) continue;
                float val = siteParam[ii + size * jj];
                if (val < 0) continue;
                if (cc && val > cc) continue;
                ci = ii; cj = jj; cc = val;
            }

        float c = (!cc) ? length + LARGE_BOUNDARY_VAL : cc + sqrt(float((pt.i - ci) * (pt.i - ci) + (pt.j - cj) * (pt.j - cj)));

        siteParam[pt.i + size * pt.j] = c;               //Also store the sites' parameterization in siteParam[] for CUDA

        if (c > length) length = c;                  //Determine length of boundary, used for wraparound-distance-computations

        for (int dj = -1; dj < 2; ++dj)
            for (int di = -1; di < 2; ++di) {
                if (di == 0 && dj == 0) continue;
                int ii = pt.i + di, jj = pt.j + dj;
                if (ii >= xm && ii < xM && jj >= ym && jj < yM && siteParam[ii + size * jj] == NARROW_BAND)
                    s.push_back(Coord(ii, jj));
            }
    }

    for (int i = 0, iend = size * size; i < iend; ++i) if (siteParam[i] < 0) siteParam[i] = 0;

    return length;
}



float skelft2DMakeBoundary(unsigned char* f, int xm, int ym, int xM, int yM, float* siteParam, int size, short iso, bool thr_upper) {
    bool  scan_bot_to_top  = false;

    vector<Coord> s; 
    s.reserve((xM - xm) * (yM - ym));

    classify(s, f, xm, ym, xM, yM, size, iso, thr_upper, siteParam, scan_bot_to_top);
   
    return encodeBoundary(s, xm, ym, xM, yM, siteParam, size);
    
}


void skelft2DSave(short* outputFT, int dx, int dy, const char* f) {
    FILE* fp = fopen(f, "wb");
    fprintf(fp, "P5 %d %d 255\n", dx, dy);

    const int SIZE = 3000;
    unsigned char buf[SIZE];

    int size = dx * dy;
    int bb   = 0;
    float range = max(dx, dy) / 255.0f;
    for (short *v = outputFT, *vend = outputFT + size; v < vend; ++v) {
        short val = *v;
        buf[bb++] = (unsigned char)(val * 0xff);
        if (bb == SIZE)
        {  fwrite(buf, sizeof(unsigned char), SIZE, fp); bb = 0; }
    }
    if (bb) fwrite(buf, sizeof(unsigned char), bb, fp);
    fclose(fp);
}

void skelft2DSave(unsigned char* outputDT, int dx, int dy, const char* f) {
    FILE* fp = fopen(f, "wb");
    fprintf(fp, "P5 %d %d 255\n", dx, dy);

    const int SIZE = 3000;
    unsigned char buf[SIZE];

    int size = dx * dy;
    int bb   = 0;
    // float range = max(dx, dy) / 255.0f;
    for (unsigned char *v = outputDT, *vend = outputDT + size; v < vend; ++v) {
        unsigned char val = *v;
        buf[bb++] = (*v);
        if (bb == SIZE)
        {  fwrite(buf, sizeof(unsigned char), SIZE, fp); bb = 0; }
    }
    if (bb) fwrite(buf, sizeof(unsigned char), bb, fp);
    fclose(fp);
}



