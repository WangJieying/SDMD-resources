//CUDA-based Skeletonization, Distance Transforms, Feature Transforms, Erosion and Dilation, and Flood Filling Toolkit
//
//(c) Alexandru Telea, Univ. of Groningen, 2011
//====================================================================================================================

#pragma once

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <GL/glut.h>
#include <map>
#include "io.h"
#include "genrl.h"



using namespace std;

class FLAGS;

template <class T> class FIELD {
public:

    enum FILE_TYPE { VTK, ASCII, PGM, BMP, PPM, UNKNOWN };

    FIELD(int = 0, int = 0);
    FIELD(const FIELD&);
    ~FIELD();
    T&            value(int, int);                //value at (i,j), with boundary checking
    const T&      value(int, int) const;          //const version of above
    void          set(int, int, T value);
    void          setAll(int, int, T*);

    T&            operator()(int, int);               //same as value but w/o boundary checking
    const T&      operator()(int, int) const;
    const T       value(float, float) const;      //bilinearly interpolated value anywhere inside field
    T     gradnorm(int, int) const;       //norm of grad at (i,j)
    void          setData(T* data) {v = data;}
    T*            data()                  { return v;  }
    const T*      data() const                    { return v;  }
    int       dimX()          { return nx; }  //number of columns
    int       dimY()          { return ny; }  //number of rows
    int       dimX() const        { return nx; }  //number of columns
    int       dimY() const        { return ny; }  //number of rows
    FIELD&    operator=(FIELD&);          //assignment op
    FIELD&    operator=(T);               //assignment op
    FIELD&        operator+=(const FIELD&);               //addition op
    FIELD&        operator-=(const FIELD&);       //subtraction op
    FIELD&        operator/=(const FIELD&);       //division op
    void      gradnorm(FIELD&) const;         //norm of grad as field
    void      minmax(T&, T&, T&) const;       //min, max, avg for field
    void      normalize();                //normalize this between 0 and 1
    FIELD&    operator*=(T);              //multiply field by scalar
    FIELD<T>*     dupe();
    void          threshold(const T);
    void          threshold_(const T);

    void      write(char*);               //write field to VTK struct points data file
    void      writeGrid(char*);           //write field to VTK struct grid data file
    void      writePPM(const char*) const;            //write field to PPM RGB file
    void      writePGM(const char*) const;    //write field to PGM grayscale file
    void      NewwritePGM(const char*) const;    //write field to PGM grayscale file
    void          display(const char* = 0);               //make a GLUT window w. title to show this
    void          display(int, const char* = 0);      //show this in already made i-th GLUT window
    static void   drawColor(bool);            //draw using rainbow colormap or b/w

    static FIELD* read(const char*);              //read field from data file in various formats
    static FILE_TYPE
    fileType(const char*);
    static void   reshape(int, int);
    void      size(const FIELD&);


private:

    static FIELD* readVTK(char*);             //read field from VTK scalar data file
    static FIELD* readPGM(char*);             //read field from PGM binary file
    static FIELD* readASCII(char*);           //read field from plain ASCII data file
    static FIELD* readBMP(char*);                         //read field from BMP image file
    static unsigned long
    getLong(fstream&);
    static unsigned short
    getShort(fstream&);
    static std::map<int, FIELD*> fields;
    static bool   draw_color;
    static void   draw_cb();
    void          draw();
    int       nx, ny;                 //nx = ncols, ny = nrows
    T*        v;
};


template <class T> std::map<int, FIELD<T>*> FIELD<T>::fields;
template <class T> bool FIELD<T>::draw_color = true;



//---------------------------------------------------------------










template <class T> inline FIELD<T>::FIELD(int nx_, int ny_): nx(nx_), ny(ny_), v((nx_ * ny_) ? new T[nx_ * ny_] : 0)
{  }

template <class T> inline FIELD<T>::FIELD(const FIELD<T>& f): nx(f.nx), ny(f.ny), v((f.nx * f.ny) ? new T[f.nx * f.ny] : 0)
{  if (nx * ny) memcpy(v, f.v, nx * ny * sizeof(T));  }

template <class T> inline T& FIELD<T>::value(int i, int j) {
    i = (i < 0) ? -i : (i >= nx) ? 2 * nx - i - 1 : i;
    j = (j < 0) ? -j : (j >= ny) ? 2 * ny - j - 1 : j;
    return *(v + j * nx + i);
}

template <class T> inline T& FIELD<T>::operator()(int i, int j)
{  return *(v + j * nx + i);  }

template <class T> inline const T& FIELD<T>::operator()(int i, int j) const
{  return *(v + j * nx + i);  }

template <class T> inline const T FIELD<T>::value(float i, float j) const {
    int       ii = int(i), jj      = int(j);              //get cell in which the floating-point

    T f1 = value(ii, jj) * (1 + jj - j)   + value(ii, jj + 1) * (j - jj);
    T f2 = value(ii + 1, jj) * (1 + jj - j) + value(ii + 1, jj + 1) * (j - jj);

    return (f2 - f1) * (i - ii) + f1;


    //const T* pij = v+jj*nx+ii;                      //falls. We shall then bilinearly interpolate
    //T f1 = (*pij)*(1+jj-j)     + (*(pij+nx))*(j-jj);            //between the 4 cell's vertex values.
    //T f2 = (*(pij+1))*(1+jj-j) + (*(pij+nx+1))*(j-jj);          //We write the expressions directly, and not in
    //terms of FIELD::value(), since it is faster
    //return (f2-f1)*(i-ii)+f1;
}


template <class T> inline void FIELD<T>::set(int i, int j, T value) {
    i = (i < 0) ? -i : (i >= nx) ? 2 * nx - i - 1 : i;
    j = (j < 0) ? -j : (j >= ny) ? 2 * ny - j - 1 : j;
    *(v + j * nx + i) = value;
}

template <class T> inline void FIELD<T>::setAll(int dX, int dY, T* d) {
    nx = dX;
    ny = dY;
    v  = d;
}


template <class T> inline const T& FIELD<T>::value(int i, int j) const {
    i = (i < 0) ? -i : (i >= nx) ? 2 * nx - i - 1 : i;
    j = (j < 0) ? -j : (j >= ny) ? 2 * ny - j - 1 : j;
    return *(v + j * nx + i);
}


template <class T> inline T FIELD<T>::gradnorm(int i, int j) const {
    T ux = value(i + 1, j) - value(i - 1, j);
    T uy = value(i, j + 1) - value(i, j - 1);
    return (ux * ux + uy * uy) / 4;
}

template <class T> inline FIELD<T>::~FIELD() {
    for (typename std::map<int, FIELD*>::iterator i = fields.begin(); i != fields.end(); i++)
        if ((*i).second == this) { fields.erase((*i).first); break; }
    delete[] v;
}

template <class T> FIELD<T>* FIELD<T>::dupe() {
    FIELD<T> *out = new FIELD<T>;
    out->nx = nx;
    out->ny = ny;
    out->v = new T[nx * ny];
    if (!out->v) { printf("Could nod duplicate field\n"); exit(-1);}
    memcpy(out->v, v, nx * ny * sizeof(T));
    return out;
}

template <class T> void FIELD<T>::threshold(const T f) {
    T *curr = v;
    int end = nx * ny;
    for (int i = 0; i < end; ++i) {
        if (*curr >= f)
        { *curr = 0.0; }
        else
        { *curr = 255.0; }
        ++curr;
    }
}
template <class T> void FIELD<T>::threshold_(const T f) {
    T *curr = v;
    int end = nx * ny;
    for (int i = 0; i < end; ++i) {
        if (*curr >= f)
        { *curr = 255.0; }
        else
        { *curr = 0.0; }
        ++curr;
    }
}
template <class T> inline void FIELD<T>::size(const FIELD& f) {
    delete[] v;
    nx = f.nx; ny = f.ny;
    v = (nx * ny) ? new T[nx * ny] : 0;
}

template <class T> FIELD<T>& FIELD<T>::operator=(FIELD& f) {
    if (nx != f.nx || ny != f.ny)
        size(f);
    if (nx * ny) memcpy(v, f.v, nx * ny * sizeof(T));
    return *this;
}

template <class T> FIELD<T>& FIELD<T>::operator+=(const FIELD& f) {
    if (f.dimX() == dimX() && f.dimY() == dimY()) {
        const T* fptr = f.data();
        for (T *vptr = v, *vend = v + nx * ny; vptr < vend; vptr++, fptr++)
            (*vptr) += (*fptr);
    }
    return *this;
}

template <class T> FIELD<T>& FIELD<T>::operator-=(const FIELD& f) {
    if (f.dimX() == dimX() && f.dimY() == dimY()) {
        const T* fptr = f.data();
        for (T *vptr = v, *vend = v + nx * ny; vptr < vend; vptr++, fptr++)
            (*vptr) -= (*fptr);
    }
    return *this;
}

template <class T> FIELD<T>& FIELD<T>::operator/=(const FIELD& f) {
    if (f.dimX() == dimX() && f.dimY() == dimY()) {
        const T* fptr = f.data();
        for (T *vptr = v, *vend = v + nx * ny; vptr < vend; vptr++, fptr++)
            if (fabs(*fptr) > 1.0e-7)
                (*vptr) /= (*fptr);
    }
    return *this;
}


template <class T> FIELD<T>& FIELD<T>::operator=(T val) {
    for (T* vptr = v, *vend = v + nx * ny; vptr < vend; vptr++)
        (*vptr) = val;
    return *this;
}

template <class T> void FIELD<T>::gradnorm(FIELD& f) const {
    f.size(*this);

    for (int j = 0; j < ny; j++)
        for (int i = 0; i < nx; i++)
            f.value(i, j) = gradnorm(i, j);
}

template <class T> void FIELD<T>::minmax(T& m, T& M, T& a) const {
    const float MYINFINITY_2 = MYINFINITY / 2;

    if (nx * ny < 2) { m = M = a = 0; return; }
    m = v[0];
    M = -T(MYINFINITY);
    a = 0;

    for (T* vptr = v, *vend = v + nx * ny; vptr < vend; vptr++) {
        if (m > *vptr) m = *vptr;
        if (M < *vptr && *vptr < MYINFINITY_2) M = *vptr;
        a += *vptr;
    }
    a /= nx * ny;
}

template <class T> void FIELD<T>::normalize() {
    const float MYINFINITY_2 = MYINFINITY / 2;

    float m, M, a, d; minmax(m, M, a);
    d = (M - m > 1.0e-5) ? M - m : 1;

    for (T* vptr = v, *vend = v + nx * ny; vptr < vend; vptr++) {
        float v = *vptr;
        *vptr = (v > M) ? 1 : (v - m) / d;
    }
}



template <class T> FIELD<T>& FIELD<T>::operator*=(T f) {
    for (T* vptr = v, *vend = v + nx * ny; vptr < vend; vptr++)
        *vptr *= f;
    return *this;
}


template <class T> FIELD<T>* FIELD<T>::read(const char* fname_) {
    char* fname = (char*)fname_;
    switch (fileType(fname)) {
    case BMP:   return readBMP(fname);
    case VTK:   return readVTK(fname);
    case PGM:   return readPGM(fname);
    case ASCII: return readASCII(fname);
    default:    return 0;
    }
}




template <class T> typename FIELD<T>::FILE_TYPE FIELD<T>::fileType(const char* fname) {
    FILE* fp = fopen(fname, "r");
    if (!fp) return UNKNOWN;

    char c1, c2;
    if (fscanf(fp, "%c%c", &c1, &c2) != 2) return UNKNOWN;
    fclose(fp);

    if (c1 == '#') return VTK;
    if (c1 == 'P' && c2 == '5') return PGM;
    if (c1 == 'P' && c2 == '6') return PPM;
    if (c1 == 'B' && c2 == 'M') return BMP;
    return ASCII;
}



template <class T> unsigned long FIELD<T>::getLong(fstream& inf) {
    unsigned long ip; char ic;
    unsigned char uc;
    inf.get(ic); uc = ic; ip = uc;
    inf.get(ic); uc = ic; ip |= ((unsigned long)uc << 8);
    inf.get(ic); uc = ic; ip |= ((unsigned long)uc << 16);
    inf.get(ic); uc = ic; ip |= ((unsigned long)uc << 24);
    return ip;
}

template <class T> unsigned short FIELD<T>::getShort(fstream& inf) {
    char ic; unsigned short ip;
    inf.get(ic); ip = ic;
    inf.get(ic); ip |= ((unsigned short)ic << 8);
    return ip;
}


template <class T> FIELD<T>* FIELD<T>::readBMP(char* fname) {
    fstream inf;
    inf.open(fname, ios::in | ios::binary);
    if (!inf) return 0;
    char ch1, ch2;
    inf.get(ch1); inf.get(ch2);                             //read BMP header
    unsigned long  fileSize = getLong(inf);
    unsigned short res1     = getShort(inf);
    unsigned short res2     = getShort(inf);
    unsigned long  offBits  = getLong(inf);
    unsigned long  hdrSize  = getLong(inf);
    unsigned long  numCols  = getLong(inf);
    unsigned long  numRows  = getLong(inf);
    unsigned short planes   = getShort(inf);
    unsigned short bitsPix  = getShort(inf);                //8 or 24 bits per pixel
    unsigned long  compr    = getLong(inf);
    unsigned long  imgSize  = getLong(inf);
    unsigned long  xPels    = getLong(inf);
    unsigned long  yPels    = getLong(inf);
    unsigned long  lut      = getLong(inf);
    unsigned long  impCols  = getLong(inf);
    int bpp = bitsPix / 8;                                  //1 or 3 bytes per pixel
    unsigned int nBytesInRow  = ((bpp * numCols + 3) / 4) * 4;
    unsigned int numPadBytes  = nBytesInRow - bpp * numCols;
    FIELD<T>* f = new FIELD<T>(numCols, numRows);
    T* data = f->data();
    unsigned char ch;
    for (unsigned int row = 0; row < numRows; row++) {      //for every row
        for (unsigned int col = 0; col < numCols; col++) {
            if (bpp == 3) {                                   //read data as RGB 'luminance'
                char r, g, b; inf.get(b); inf.get(g); inf.get(r);
                *data++ = (int((unsigned char)r) + int((unsigned char)g) + int((unsigned char)b)) / 3;
            } else                                            //read data as 8-bit luminance
            {  char x; inf.get(x); *data++ = (unsigned char)x; }
        }
        for (unsigned int k = 0; k < numPadBytes; k++) inf >> ch; //skip pad bytes at end of row
    }
    inf.close();
    return f;
}



template <class T> FIELD<T>* FIELD<T>::readVTK(char* fname) { //read VTK scalar file into this
    FILE* fp = fopen(fname, "r");
    if (!fp) return 0;

    char buf[100];

    FIELD<T>* f = 0;

    for (; fscanf(fp, "%s", buf) == 1;) {
        if (!strcmp(buf, "DIMENSIONS")) {
            int dimX, dimY;
            fscanf(fp, "%d%d", &dimX, &dimY);
            f = new FIELD<T>(dimX, dimY);
        }

        if (!strcmp(buf, "LOOKUP_TABLE")) {
            fscanf(fp, "%*s");
            break;
        }
    }

    for (T* d = f->data(); fscanf(fp, "%f", d) == 1; d++);

    fclose(fp);
    return f;
}



template <class T> FIELD<T>* FIELD<T>::readPGM(char* fname) { //read VTK scalar file into this
    FILE* fp = fopen(fname, "rb"); if (!fp) return 0;

    const int SIZE = 1024;
    char buf[SIZE]; int dimX, dimY, range;
    fscanf(fp, "%*s");               //skip "P5" header

    for (;;) {
        fscanf(fp, "%s", buf);         //get dimX or #comment
        if (buf[0] == '#') fgets(buf, SIZE, fp);
        else { dimX = atoi(buf); break; }
    }
    for (;;) {
        fscanf(fp, "%s", buf);         //get dimY or #comment
        if (buf[0] == '#') fgets(buf, SIZE, fp);
        else { dimY = atoi(buf); break; }
    }
    for (;;) {
        fscanf(fp, "%s", buf);         //get range or #comment
        if (buf[0] == '#') fgets(buf, SIZE, fp);
        else { range = atoi(buf); break; }
    }


    FIELD<T>* f = new FIELD<T>(dimX, dimY);
    int bb = SIZE; fgets(buf, SIZE, fp);

    for (T *d = f->data(), *end = d + dimX * dimY; d < end; d++) { //read the binary data into the field
        //be careful: buf is a char, we first need
        if (bb == SIZE) { fread(buf, SIZE, 1, fp); bb = 0; } //to convert the read bytes to unsigned char and then assign
        *d = (unsigned char)buf[bb++];              //to the field!
    }

    fclose(fp);
    return f;
}



template <class T> FIELD<T>* FIELD<T>::readASCII(char* fname) { //read plain ASCII file into this
    FILE* fp = fopen(fname, "r");
    if (!fp) return 0;

    FIELD<T>* f = 0;

    int dimX, dimY, dimZ;
    fscanf(fp, "%d%d%d", &dimX, &dimY, &dimZ);
    f = new FIELD<T>(dimX, dimY);

    for (T* d = f->data(); fscanf(fp, "%f", d) == 1; d++);

    fclose(fp);
    return f;
}




template <class T> void FIELD<T>::write(char* fname) {
    FILE* fp = fopen(fname, "w");
    if (!fp) return;

    fprintf(fp, "# vtk DataFile Version 2.0\n"
            "vtk output\n"
            "ASCII\n"
            "DATASET STRUCTURED_POINTS\n"
            "DIMENSIONS %d %d 1\n"
            "SPACING 1 1 1\n"
            "ORIGIN 0 0 0\n"
            "POINT_DATA %d\n"
            "SCALARS scalars float\n"
            "LOOKUP_TABLE default\n",
            nx, ny, nx * ny);

    for (T* vend = v + nx * ny, *vptr = v; vptr < vend; vptr++)
        fprintf(fp, "%f\n", float(*vptr));

    fclose(fp);
}


template <class T> void FIELD<T>::writeGrid(char* fname) {
    FILE* fp = fopen(fname, "w");
    if (!fp) return;

    fprintf(fp, "# vtk DataFile Version 2.0\n"
            "vtk output\n"
            "ASCII\n"
            "DATASET STRUCTURED_GRID\n"
            "DIMENSIONS %d %d 1\n"
            "POINTS %d int\n",
            nx, ny, nx * ny);

    for (int j = 0; j < ny; j++)
        for (int i = 0; i < nx; i++)
            fprintf(fp, "%d %d 0\n", i, j);

    fprintf(fp, "POINT_DATA %d\n"
            "SCALARS scalars float\n"
            "LOOKUP_TABLE default\n",
            nx * ny);

    for (T* vend = v + nx * ny, *vptr = v; vptr < vend; vptr++)
        fprintf(fp, "%f\n", float(*vptr));

    fclose(fp);
}


template <class T> void FIELD<T>::writePGM(const char* fname) const {
    FILE* fp = fopen(fname, "wb");
    if (!fp) return;

    float m, M; T m_, M_, avg_; minmax(m_, M_, avg_);
    m = m_; M = M_;

    const int SIZE = 3000;
    unsigned char buf[SIZE];
    int bb = 0;

    fprintf(fp, "P5 %d %d 255\n", dimX(), dimY());
    for (const T* vend = data() + dimX() * dimY(), *vptr = data(); vptr < vend; vptr++) {
        float v = ((*vptr) - m) / (M - m); v = max(v, 0);
        if (v > M) v = 1; else v = min(v, 1);
        buf[bb++] = (unsigned char)(int)(v * 255);
        if (bb == SIZE)
        {  fwrite(buf, sizeof(unsigned char), SIZE, fp); bb = 0; }
    }
    if (bb) fwrite(buf, sizeof(unsigned char), bb, fp);

    fclose(fp);
}

template <class T> void FIELD<T>::NewwritePGM(const char* fname) const {//T should be 'int' type.
    FILE* fp = fopen(fname, "wb");
    if (!fp) return;

    const int SIZE = 3000;
    unsigned char buf[SIZE];
    int bb = 0;

    fprintf(fp, "P5 %d %d 255\n", dimX(), dimY());
    for (const T* vend = data() + dimX() * dimY(), *vptr = data(); vptr < vend; vptr++) {
        
        buf[bb++] = (unsigned char)(*vptr);
        if (bb == SIZE)
        {  fwrite(buf, sizeof(unsigned char), SIZE, fp); bb = 0; }
    }
    if (bb) fwrite(buf, sizeof(unsigned char), bb, fp);

    fclose(fp);
}

template <class T> void FIELD<T>::writePPM(const char* fname) const {
    FILE* fp = fopen(fname, "wb");
    if (!fp) return;

    float m, M; T m_, M_, avg_; minmax(m_, M_, avg_);
    m = m_; M = M_;

    const int SIZE = 3000;
    unsigned char buf[SIZE];
    int bb = 0;

    fprintf(fp, "P6 %d %d 255\n", dimX(), dimY());
    for (const T* vend = data() + dimX() * dimY(), *vptr = data(); vptr < vend; vptr++) {
        float r, g, b, v = ((*vptr) - m) / (M - m);
        v = max(v, 0);
        if (v > M) { r = g = b = 1; } else v = min(v, 1);
        float2rgb(v, r, g, b);

        buf[bb++] = (unsigned char)(int)(r * 255);
        buf[bb++] = (unsigned char)(int)(g * 255);
        buf[bb++] = (unsigned char)(int)(b * 255);
        if (bb == SIZE)
        {  fwrite(buf, 1, SIZE, fp); bb = 0; }
    }
    if (bb) fwrite(buf, 1, bb, fp);

    fclose(fp);
}

template <class T> void FIELD<T>::display(const char* t) {
    int w = glutCreateWindow((t) ? t : "Field Display");
    glutDisplayFunc(draw_cb);
    glutReshapeFunc(reshape);
    typename std::map<int, FIELD*>::value_type v(w, this);
    fields.insert(v);
}

template <class T> void FIELD<T>::drawColor(bool b)
{  draw_color = b; glutPostRedisplay(); }

template <class T> void FIELD<T>::display(int id, const char* t) {
    int cnt = 0;
    typename std::map<int, FIELD*>::iterator i;
    for (i = fields.begin(); cnt < id && i != fields.end(); i++, cnt++);
    if (i == fields.end()) return;

    int w = (*i).first;
    fields.erase(i);
    typename std::map<int, FIELD*>::value_type v(w, this);
    fields.insert(v);
    if (t) { glutSetWindow(w); glutSetWindowTitle(t); }
}

template <class T> void FIELD<T>::draw_cb() {
    typename std::map<int, FIELD*>::iterator i = fields.find(glutGetWindow());
    if (i != fields.end()) (*i).second->draw();
}

template <class T> void FIELD<T>::draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    static unsigned char buf[800 * 800 * 3];
    float m, M, avg; minmax(m, M, avg);
    const float* d = data(); int j = 0;
    for (int i = dimY() - 1; i >= 0; i--)
        for (const float *s = d + dimX() * i, *e = s + dimX(); s < e; s++) {
            float r, g, b, v = ((*s) - m) / (M - m);
            v = (v > 0) ? v : 0;
            if (v > M) { r = g = b = 1; } else v = (v < 1) ? v : 1;
            if (draw_color)
                float2rgb(v, r, g, b);
            else
                r = g = b = 1 - v;
            buf[j++] = (unsigned char)(int)(255 * r);
            buf[j++] = (unsigned char)(int)(255 * g);
            buf[j++] = (unsigned char)(int)(255 * b);
        }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glDrawPixels(dimX(), dimY(), GL_RGB, GL_UNSIGNED_BYTE, buf);
    glutSwapBuffers();
}

template <class T> void FIELD<T>::reshape(int w, int h) {
    glViewport(0.0f, 0.0f, (GLfloat)w, (GLfloat)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (GLdouble)w, 0.0, (GLdouble)h);
}
