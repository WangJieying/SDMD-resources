/*
 * File:   ImageDecoder.hpp
 * Author: yuri
 *
 * Created on June 13, 2011, 1:32 PM
 */

#ifndef IMAGEDECODER_HPP
#define IMAGEDECODER_HPP

#include <vector>
#include <algorithm>
#include <map>
#include <sstream>
#include "Triple.hpp"
#include "fileio/fileio.hpp"
#include "FastAC/arithmetic_codec.h"

using namespace std;

typedef Triple<int, int, int> coord3D_t;
typedef vector< coord3D_t > layer_t;
typedef layer_t path_t;
typedef vector<layer_t*> image_t;


class ImageDecoder {
public:
    /*******************************************************************************/
    /*******************************************************************************/
    /** IMPORTANT -- WHEN CHANGES ARE MADE TO THE FILE FORMAT, UPDATE THIS NUMBER **/
    uint16_t WRITER_FILE_VERSION_NUMBER = 0xB;
    uint16_t READER_FILE_VERSION_NUMBER = 0xB;
    /*******************************************************************************/
    /*******************************************************************************/

    ImageDecoder();
    ImageDecoder(const ImageDecoder& orig);
    virtual ~ImageDecoder();
    COLORSPACE load(const char *file);
    void loadSample(int SuperResolution, int colorSpace);
    int decompress_data(unsigned char *compr_data, int length, unsigned char **out,  unsigned int *outlength);
    image_t* getImage() const {
        return r_image;
    }
    image_t* getGChannel() const {
        return g_image;
    }
    image_t* getBChannel() const {
        return b_image;
    }

    vector<int>& get_gray_levels() {
        return gray_levels;
    };
    vector<int>& get_g_levels() {
        return g_gray_levels;
    };
    vector<int>& get_b_levels() {
        return b_gray_levels;
    };

    int width, height;
    bool Yremoval = false;
    //  bool Yremoval = true;
    int Rfirst,Gfirst,Bfirst;
    vector<int> Invert, g_Invert, b_Invert;
private:
    unsigned char *data = 0, *dat_it;
    image_t* r_image = nullptr;
    image_t* g_image = nullptr;
    image_t* b_image = nullptr;
    vector<int> gray_levels, g_gray_levels, b_gray_levels;
    
    vector<tuple<int, int, int>> point_list;
    //ofstream of_uncompressed;
    enum COMPRESS_MODE {COMPRESS_LZMA = 0, COMPRESS_ZLIB, COMPRESS_LZHAM, COMPRESS_LZMA2, COMPRESS_BROTLI, COMPRESS_ZPAQ, COMPRESS_BSC, COMPRESS_CSC, COMPRESS_BZIP2, COMPRESS_ZSTD};
    enum ENCODE_MODE {HUFFMAN = 1, CANONICAL, UNITARY, EXP_GOULOMB, ARITHMETIC, TRADITIONAL};
    const char* get_comp_method_string(COMPRESS_MODE mode);
    void addPoint(char point, int &x, int &y, int &r, path_t *path);
    size_t read_layer(unsigned char* dat_it, path_t *layer);
    int bits_dx, bits_dy, bits_dr;
    bool decode_layer(image_t** image, int intensity);
    image_t* decode_plane(vector<int>& levels);


};

#endif  /* IMAGEDECODER_HPP */

