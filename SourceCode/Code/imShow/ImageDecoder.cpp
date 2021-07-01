/*
 * File:   ImageDecoder.cpp
 * Author: yuri
 *
 * Created on June 13, 2011, 1:32 PM
 */

#include <vector>
#include "./include/io.hpp"
#include "./include/ImageDecoder.hpp"
#include "./include/messages.h"
#include "./include/DataManager.hpp"
#include "../shared/FastAC/arithmetic_codec.h"
#include "squash/squash.h"
#include "fileio/fileio.hpp"
#include <assert.h>
#include <fstream>
#include <bitset>
#include <climits>
#include <math.h>

using namespace std;

ImageDecoder::ImageDecoder() {
    width = -1;
    height = -1;
}

ImageDecoder::ImageDecoder(const ImageDecoder& orig) {
}

int MAXR = 0;

ImageDecoder::~ImageDecoder() {
}

/* Add a point to the current datatype. This is used for both starting points, and neighbouring points.
 * Identify a starting point by setting "point" to 0. This cannot happen for neighbouring points, as that
 * implicates the radius is 0.*/
void ImageDecoder::addPoint(char point, int &x, int &y, int &r, path_t *path) {
    /* If we are a neighbouring point: decode the character and walk in the direction. Modify the original
     * x,y,r values. */
    // Layout of point is 0xxyyrrr where xx are the bits denoting delta_x [0, 1, 2]
    // yy denotes delta_y [0, 1, 2] and rrr denote delta_radius [0, 1, 2, 3, 4]
    int8_t dx = ((point >> 5) & 0x3) - 1;
    int8_t dy = ((point >> 3) & 0x3) - 1;
    int8_t dr = (point & 0x7) - 2;
    x += dx;
    y += dy;
    r += dr;


    // if (r < 0) { cerr << "Invalid radius size, must be >0 [r=" << r << "]" << endl; exit(-1);}

    /* Add to datatype */
    path->push_back(coord3D_t(x, y, r));

}

const char* ImageDecoder::get_comp_method_string(COMPRESS_MODE mode) {
    switch (mode) {
    case COMPRESS_ZLIB:
        return "zlib";
        break;
    case COMPRESS_LZHAM:
        return "lzham";
        break;
    case COMPRESS_BSC:
        return "bsc";
        break;
    case COMPRESS_CSC:
        return "csc";
        break;
    case COMPRESS_LZMA:
        return "xz";
        break;
    case COMPRESS_LZMA2:
        return "lzma2";
        break;
    case COMPRESS_BROTLI:
        return "brotli";
        break;
    case COMPRESS_ZPAQ:
        return "zpaq";
        break;
    case COMPRESS_BZIP2:
        return "bzip2";
        break;
    case COMPRESS_ZSTD:
        return "zstd";
        break;
    default:
        // Uses some unsupported method or file is mangled.
        return NULL;
        break;
    }
}

/* Decode LZMA. */
int ImageDecoder::decompress_data(unsigned char *compr_data, int length, unsigned char **out, unsigned int *outlength) {
    unsigned char *dat_it = compr_data;
    unsigned char *decompr_data;
    size_t decompr_length = -1;
    size_t compr_length = length;
    /* First byte stores the compression method. */
    int compression_method = (int)dat_it[0];
    const char* comp_method_name = get_comp_method_string((ImageDecoder::COMPRESS_MODE)compression_method);
    dat_it += 1; compr_length -= 1;

    /* First 4 bytes store the size of the uncompressed data. */
    decompr_length = *((unsigned int *) dat_it);
    decompr_data = new unsigned char[decompr_length];

    dat_it += 4; compr_length -= 4;

    // TODO: FIX THIS SO IT ALWAYS WORKS!
    squash_set_default_search_path("../shared/squash/plugins");

    SquashCodec* codec = squash_get_codec(comp_method_name);
    if (codec == NULL) {
        fprintf (stderr, "Unable to find algorithm '%s'.\n", comp_method_name);
        delete [] decompr_data;
        return EXIT_FAILURE;
    }


    PRINT(MSG_NORMAL, "%s Input size: %f KB, Uncompressed output: %fKB\n", comp_method_name, length / 1024.0, decompr_length / 1024.0);
    SquashStatus res = squash_codec_decompress (codec,
                       &decompr_length, (uint8_t*) decompr_data,
                       compr_length, dat_it, NULL);

    switch (res) {
    case SQUASH_OK:
        cout << comp_method_name << " decompression went OK." << endl;
        break;
    // TODO(maarten): MORE CASES HERE!
    default:
        cout << comp_method_name << " returned unknown return value" << endl;
    }
    /* Fill return values */
    *out = decompr_data;
    *outlength = decompr_length;

    //free(compr_data);
    
    return res;
}

bool ImageDecoder::decode_layer(image_t** image_ref, int intensity) {
    // uint8_t x1 = READUINT8(dat_it);
    // uint8_t x2 = READUINT8(dat_it);
   if (intensity == 255) intensity = 254;//Wang. I don't know why when the intensity equals to 255, it doesn't work, so here I just change it to 254 since it isn't distinguishable.
    image_t* image = *image_ref;
    path_t* layer = (*image)[intensity];
    // uint16_t numPaths =  (x1 << 8) + x2;
    uint16_t numPaths = READUINT16(dat_it);
    uint8_t current;
    if (numPaths == 0xFFFF)
        return false;

    //of_uncompressed << "Intensity " << +intensity << " - " << "Num children " << +numPaths << endl;
    
    for (int i = 0; i < numPaths; ++i) {
     
        path_t* path = new path_t();
        /* Read first -full- point */
        /* Seriously, y tho*/
        uint8_t x1 = READUINT8(dat_it);
        uint8_t x2 = READUINT8(dat_it);
        int x = (x1 << 8) + x2;
        uint8_t y1 = READUINT8(dat_it);
        uint8_t y2 = READUINT8(dat_it);
        int y = (y1 << 8) + y2;
        uint8_t r1 = READUINT8(dat_it);
        uint8_t r2 = READUINT8(dat_it);
        int r = (r1 << 8) + r2;
        path->push_back(coord3D_t(x, y, r));
      
        bool end = false;
        while (!end) {
            //of_uncompressed << x << " - " << y << " - " << r << endl;
            current = READUINT8(dat_it);
            if (current == END_TAG) {
                //of_uncompressed << "End" << endl;
                end = true;
            }

            /* End of branch? */
            else if (current == FORK_TAG) {
                uint8_t goBack_1 = READUINT8(dat_it);
                uint8_t goBack_2 = READUINT8(dat_it);
                uint16_t goBack = (goBack_1 << 8) + goBack_2; // TODO(maarten): why is this necessary?
                //of_uncompressed << "Fork - " << (goBack) << endl;
                for (unsigned int q = 0; q < goBack; ++q) {
                    layer->push_back((path->back()));
                    path->pop_back();
                }
                x = path->back().first;
                y = path->back().second;
                r = path->back().third;
            } else {
                if ((current < 128)) {
                    addPoint(current, x, y, r, path);
                } else {
                    if (current == 128) {
                        if (bits_dx == 0) {
                            int8_t dx_8 = READINT8(dat_it);
                            x += dx_8;
                        } else {
                            uint8_t dx_1_16 = READUINT8(dat_it);
                            uint8_t dx_2_16 = READUINT8(dat_it);
                            x += ((dx_1_16 << 8) + dx_2_16);
                            if (x > width)
                                x -= (1 << 16);
                        }
                        if (bits_dy == 0) {
                            int8_t dy_8 = READINT8(dat_it);
                            y += dy_8;
                        } else {
                            uint8_t dy_1_16 = READUINT8(dat_it);
                            uint8_t dy_2_16 = READUINT8(dat_it);
                            y += ((dy_1_16 << 8) + dy_2_16);
                            if (y > width)
                                y -= (1 << 16);

                        }
                        if (bits_dr == 0) {
                            int8_t dr_8 = READINT8(dat_it);
                            r += dr_8;
                        } else {
                            uint8_t dr_1_16 = READUINT8(dat_it);
                            uint8_t dr_2_16 = READUINT8(dat_it);
                            r += ((dr_1_16 << 8) + dr_2_16);
                            if (r > width)
                                r -= (1 << 16);

                        }
                    } else {
                        uint16_t num = (current << 8) + READUINT8(dat_it);
                        int8_t dr = ((num & 0x1F)) - 15;
                        int8_t dy = (((num >> 5) & 0x1F)) - 15;
                        int8_t dx = (((num >> 10) & 0x1F)) - 15;
                        x += dx;
                        y += dy;
                        r += dr;
                    }

                    if (r < 0) { cerr << "Invalid radius size, must be >0 [r=" << r << "]" << endl; exit(-1);}

                    /* Add to datatype */
                    path->push_back(coord3D_t(x, y, r));
                }
            }

        }
        
        /* Copy path to the layer of the image */
        for (unsigned int i = 0; i < path->size(); ++i) {
                
            layer->push_back((*path)[i]);
        }

    }
    //PRINT(MSG_NORMAL, "33\n");
    return true;
}

image_t* ImageDecoder::decode_plane(vector<int>& levels) {
    image_t* img = new image_t();
   
    levels.clear();
    int num_levels = READUINT8(dat_it);
    cout<<"num_levels "<<num_levels<<endl;
    for (int i = 0; i < num_levels; ++i) {
        int level = READUINT8(dat_it);
        //cout<<"level"<<level<<endl;
        if (i == 0)
            {levels.push_back(level);}
        else
            {levels.push_back(levels.at(i - 1) + level);}
            
    }
    for (int i = 0; i < 0xff; ++i) {
        path_t *p = new path_t();
        img->push_back(p);
    }
    //of_uncompressed.open("uncompressed.sir", ios_base::out | ios::binary);
    //PRINT(MSG_NORMAL, "1\n");
    for (auto intensity : levels) {
        decode_layer(&img, intensity);
    }
    //of_uncompressed.close();
    return img;
}


/* Load SIR file, decode compressed data, and read all disks. */
COLORSPACE ImageDecoder::load(const char *fname) {
    unsigned int nEl = readFile<unsigned char>(fname, &data);
    if (nEl == 0) {
        PRINT(MSG_ERROR, "Could not open file for Image Decoder.\n");
        exit(1);
    }

    //int UseSquash = (int)READUINT8(data);//First Byte store the sign that if we use squash compression.
    
    //if (UseSquash!=48) //!='0':if use.
    decompress_data(data, nEl, &data, &nEl);// Decode compressed data, and overwrite old data with the decompressed data

    // Initialize iterator 
    dat_it = data;

    ofstream OutFile;
    OutFile.open("ControlPoints.txt");//wang
    int x,y,dt;
    int last_x=0, last_y=0, last_dt=0;
    uint8_t x1,x2;
    int Cspace = 1;

    x1 = READUINT8(dat_it);
    x2 = READUINT8(dat_it);
    x = ((x1 << 8) + x2);//width
    x1 = READUINT8(dat_it);
    x2 = READUINT8(dat_it);
    y = ((x1 << 8) + x2);//height
    OutFile<<x<<" "<<y<<endl;

    COLORSPACE colorspace = (COLORSPACE)READUINT8(dat_it);

    if (colorspace == COLORSPACE::GRAY)  {Rfirst = READUINT8(dat_it);} 
    else 
    {
        Rfirst = READUINT8(dat_it);
        Gfirst = READUINT8(dat_it);
        Bfirst = READUINT8(dat_it);
        Cspace = 3;
    }
    

  for (int i=0; i<Cspace; i++)
  {
    while (true)//each loop reads each layer.
    {
        //last_x = 0; last_y = 0; last_dt = 0;
        uint8_t firstB = READUINT8(dat_it);//layernum
        //cout<<"firstB: "<<(int)firstB<<endl;
        if ((int)firstB == 255 || (int)firstB == 0) {OutFile<<255<<endl; break;} //if there's nothing to read, then firstB would be 0.
        switch (i)
        {
            case 0:
                gray_levels.push_back((int)firstB);
                break;
            case 1:
                g_gray_levels.push_back((int)firstB);
                break;
            case 2:
                b_gray_levels.push_back((int)firstB);
                break;
            default:
                break;
        }
        
        OutFile<<(int)firstB<<endl;//layerNum

        uint8_t invertOrnot = READUINT8(dat_it);
        //cout<<" invertOrnot: "<<(int)invertOrnot;
        switch (i)
        {
            case 0:
                Invert.push_back((int)invertOrnot);
                break;
            case 1:
                g_Invert.push_back((int)invertOrnot);
                break;
            case 2:
                b_Invert.push_back((int)invertOrnot);
                break;
            default:
                break;
        }

        while(true)//each loop for each layer.
        {
            uint8_t firstB = READUINT8(dat_it);
            if ((int)firstB == 0)
            {
                OutFile<<(int)firstB<<endl;//end sign.
                break;
            } 
            int degree = (int) (firstB  & 0xF);
            int NumCP = (int) ((firstB >> 4) & 0xF);
            
            OutFile<<NumCP<<" "<<degree<<" ";

            x1 = READUINT8(dat_it);
            x2 = READUINT8(dat_it);
            x = (x1 << 8) + x2;
            OutFile<<x<<" ";//numSample
            while(NumCP--)
            {
                x1 = READUINT8(dat_it);
                x2 = READUINT8(dat_it);
                x = ((x1 << 8) + x2);
                if (x == 0)
                {
                    x1 = READUINT8(dat_it);
                    x2 = READUINT8(dat_it);
                    x = ((x1 << 8) + x2);
                    x = x > 32768 ? (x-65536) : x;
                    x += last_x;

                    x1 = READUINT8(dat_it);
                    x2 = READUINT8(dat_it);
                    y = (x1 << 8) + x2;
                    y = y > 32768 ? (y-65536) : y;
                    y += last_y;

                    x1 = READUINT8(dat_it);
                    x2 = READUINT8(dat_it);
                    dt = (x1 << 8) + x2;
                    dt = dt > 32768 ? (dt-65536) : dt;
                    dt += last_dt;
                }
                else
                {
                    x = (int)x1;
                    x = x > 128 ? (x-256) : x;
                    x += last_x;

                    y = (int)x2;
                    y = y > 128 ? (y-256) : y;
                    y += last_y;

                    x1 = READUINT8(dat_it);
                    dt = (int)x1;
                    dt = dt > 128 ? (dt-256) : dt;
                    dt += last_dt;
                }
                last_x = x;
                last_y = y;
                last_dt = dt;
                OutFile<<x<<" "<<y<<" "<<dt<<" ";
            }
            OutFile<<endl;
        }
    }
  }
    OutFile.close();
    return colorspace;

}

void ImageDecoder::loadSample(int SuperResolution, int colorSpace) {////
    
    image_t* img = new image_t();
    image_t* g_img = new image_t();
    image_t* b_img = new image_t();
    path_t* layer;

    int intensity;
    int x,y,dt,imp;

    for (int i = 0; i < 0xff; ++i) {
        path_t *p = new path_t();
        img->push_back(p);
    }
    if(colorSpace > 1){
        for (int i = 0; i < 0xff; ++i) {
            path_t *k = new path_t();
            path_t *q = new path_t();//note use a different path_t.
            g_img->push_back(k);
            b_img->push_back(q);
        }
    }
    //of_uncompressed.open("uncompressed.sir", ios_base::out | ios::binary);

    ifstream ifs("sample.txt"); 
    string str;
    ifs >> str;
    width = (int)atof(str.c_str());
    //cout<<"width: "<<width<<endl;
    ifs >> str;
    height = (int)atof(str.c_str()); 
    
    for(int i=0;i<colorSpace;i++)
    {
        ifs >> str;
        if((int)atof(str.c_str())==65536) break;

        while(true)
        {
            intensity = (int)atof(str.c_str());
            //of_uncompressed << "Intensity " << +intensity << endl;
            //cout<<"intensity: "<<intensity<<endl;
            switch (i)
            {
                case 0:
                    layer = (*img)[intensity];
                    break;
                case 1:
                    layer = (*g_img)[intensity];
                    break;
                case 2:
                    layer = (*b_img)[intensity];
                    break;
                default:
                    break;
            }
            //cout<<"layer.size() "<<layer->size()<<endl;
            while(true)
            {
                ifs >> str;
                x = (round)(atof(str.c_str()));
            
                if (x != 65535)
                { 
                    x = x*SuperResolution;
                    ifs >> str;
                    y = (round)(atof(str.c_str())*SuperResolution);
                    ifs >> str;
                    dt = (round)(atof(str.c_str())*SuperResolution);
                    //of_uncompressed << x << " - " << y << " - " << dt << endl;
                    if(Yremoval){ 
                        int leftExtension = width*0.25;
                        int topExtension = height*0.25;
                        layer->push_back(coord3D_t(x+leftExtension, y+topExtension, dt));
                    } 
                    else layer->push_back(coord3D_t(x, y, dt));

                }
                else break;
            }
            ifs >> str;
            if((int)atof(str.c_str())==65536) break;
        }
    }   
    
    //of_uncompressed.close();
    ifs.close();
    r_image = img;
    if(colorSpace > 1){
        g_image = g_img;
        b_image = b_img;
    }
}
/*
*/