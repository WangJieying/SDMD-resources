#pragma once



#include <fstream>
#include "../shared/CUDASkel2D/include/field.h"

using namespace std;


template <class T> class IMAGE {
public:
    IMAGE(int x = 0, int y = 0): r(x, y), g(x, y), b(x, y) {}
    IMAGE(const IMAGE& i): r(i.r), g(i.g), b(i.b)  {}
    void        setValue(int, int, T);
    T           value(int, int, int);
    void        set(int, int, int, T);
    void        size(int i, int j)       { r.size(i, j); g.size(i, j); b.size(i, j); }
    void        size(const FIELD<T>& f)  { r.size(f);    g.size(f);    b.size(f); }

    FIELD<T>    r, g, b;
    int     dimX() const            { return r.dimX(); }
    int     dimY() const            { return r.dimY(); }
    void    normalize()             { r.normalize(); g.normalize(); b.normalize(); }

    void writePPM(const char* file);

    static IMAGE*   read(const char*);

private:
    static IMAGE*   readBMP(const char*);
    static IMAGE*   readPPM(const char*);
    static unsigned long  getLong(fstream&);
    static unsigned short getShort(fstream&);
};


template <class T> IMAGE<T>* IMAGE<T>::read(const char* fname) {
    switch (FIELD<T>::fileType(fname)) {
    case FIELD<T>::FILE_TYPE::BMP:
        return readBMP(fname);
    case FIELD<T>::FILE_TYPE::PPM:
        return readPPM(fname);
    default:
        return 0;
    }
}

template <class T> inline void IMAGE<T>::setValue(int i, int j, T v) {
    r.value(i, j) = g.value(i, j) = b.value(i, j) = v;
}

template <class T> inline void IMAGE<T>::set(int level, int i, int j, T v) {
    if (level == 0)
        r.value(i, j) = v;
    else if (level == 1)
        g.value(i, j) = v;
    else
        b.value(i, j) = v;
}

template <class T> T IMAGE<T>::value(int level, int i, int j) {
    if (level == 0)
        return r.value(i, j);
    else if (level == 1)
        return g.value(i, j);
    else
        return b.value(i, j);
}


template <class T> IMAGE<T>* IMAGE<T>::readBMP(const char* fname) {
    fstream inf;
    inf.open(fname, ios::in | ios::binary);
    if (!inf) return 0;
    char ch1, ch2;
    inf.get(ch1); inf.get(ch2);                             // read BMP header
    unsigned long  fileSize = getLong(inf);
    unsigned short res1     = getShort(inf);
    unsigned short res2     = getShort(inf);
    unsigned long  offBits  = getLong(inf);
    unsigned long  hdrSize  = getLong(inf);
    unsigned long  numCols  = getLong(inf);
    unsigned long  numRows  = getLong(inf);
    unsigned short planes   = getShort(inf);
    unsigned short bytesPix = getShort(inf);                // 8 or 24
    unsigned long  compr    = getLong(inf);
    unsigned long  imgSize  = getLong(inf);
    unsigned long  xPels    = getLong(inf);
    unsigned long  yPels    = getLong(inf);
    unsigned long  lut      = getLong(inf);
    unsigned long  impCols  = getLong(inf);
    int bpp = bytesPix / 8;                                 // 1 or 3
    unsigned int nBytesInRow  = ((bpp * numCols + 3) / 4) * 4;
    unsigned int numPadBytes  = nBytesInRow - bpp * numCols;
    IMAGE<T>* f = new IMAGE<T>(numCols, numRows);
    T *rd = f->r.data(), *gd = f->g.data(), *bd = f->b.data();
    unsigned char ch;
    for (unsigned int row = 0; row < numRows; row++) {      // for every row
        for (unsigned int col = 0; col < numCols; col++) {
            if (bpp == 3) {                                   // read data as RGB colors
                char R, G, B; inf.get(B); inf.get(G); inf.get(R);
                *rd++ = (unsigned char)R; *gd++ = (unsigned char)G; *bd++ = (unsigned char)B;
            } else                                            // read data as 8-bit luminance
            {  char ch; inf.get(ch); *rd++ = (unsigned char)ch; *gd++ = (unsigned char)ch; *bd++ = (unsigned char)ch; }
        }
        for (unsigned int k = 0; k < numPadBytes; k++) inf >> ch; // skip pad bytes at end of row
    }
    inf.close();
    return f;
}



template <class T> IMAGE<T>* IMAGE<T>::readPPM(const char* fname) {
    FILE* fp = fopen(fname, "r"); if (!fp) return 0;

    const int SIZE = 1024 * 3;
    char buf[SIZE]; int dimX, dimY, range;
    fscanf(fp, "%*s");         //skip "P5" header

    for (;;) {
        fscanf(fp, "%s", buf);      //get dimX or #comment
        if (buf[0] == '#') fgets(buf, SIZE, fp);
        else { dimX = atoi(buf); break; }
    }
    for (;;) {
        fscanf(fp, "%s", buf);      //get dimY or #comment
        if (buf[0] == '#') fgets(buf, SIZE, fp);
        else { dimY = atoi(buf); break; }
    }
    for (;;) {
        fscanf(fp, "%s", buf);      //get range or #comment
        if (buf[0] == '#') fgets(buf, SIZE, fp);
        else { range = atoi(buf); break; }
    }


    int bb = SIZE; fgets(buf, SIZE, fp);

    IMAGE<T>* f = new IMAGE<T>(dimX, dimY);

    for (T *rd = f->r.data(), *gd = f->g.data(), *bd = f->b.data(), *end = rd + dimX * dimY; rd < end; rd++, gd++, bd++) { //read the binary data into the field
        //be careful: buf is a char, we first need
        if (bb == SIZE) { fread(buf, SIZE, 1, fp); bb = 0; } //to convert the read bytes to unsigned char and then assign
        *rd = (unsigned char)buf[bb++] / float(range);          //to the field!
        *gd = (unsigned char)buf[bb++] / float(range);
        *bd = (unsigned char)buf[bb++] / float(range);
    }

    fclose(fp);
    return f;
}



template <class T> void IMAGE<T>::writePPM(const char* file) {
    FILE* fp = fopen(file, "w");
    if (!fp) return;

    const int SIZE = 3000;
    unsigned char buf[SIZE];
    int bb = 0;

    fprintf(fp, "P6 %d %d 255\n", dimX(), dimY());
    for (const T* rend = r.data() + dimX() * dimY(), *rv = r.data(), *gv = g.data(), *bv = b.data(); rv < rend; ++rv, ++gv, ++bv) {
        buf[bb++] = (unsigned char)(*rv * 0xff);
        buf[bb++] = (unsigned char)(*gv * 0xff);
        buf[bb++] = (unsigned char)(*bv * 0xff);
        if (bb == SIZE)
        {  fwrite(buf, 1, SIZE, fp); bb = 0; }
    }
    if (bb) fwrite(buf, 1, bb, fp);

    fclose(fp);
}

template <class T> unsigned long IMAGE<T>::getLong(fstream& inf) {
    unsigned long ip; char ic;
    unsigned char uc;
    inf.get(ic); uc = ic; ip = uc;
    inf.get(ic); uc = ic; ip |= ((unsigned long)uc << 8);
    inf.get(ic); uc = ic; ip |= ((unsigned long)uc << 16);
    inf.get(ic); uc = ic; ip |= ((unsigned long)uc << 24);
    return ip;
}

template <class T> unsigned short IMAGE<T>::getShort(fstream& inf) {
    char ic; unsigned short ip;
    inf.get(ic); ip = ic;
    inf.get(ic); ip |= ((unsigned short)ic << 8);
    return ip;
}



