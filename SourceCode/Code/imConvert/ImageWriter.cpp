/*
 * File:   ImageWriter.cpp
 * Author: yuri
 *
 * Created on June 6, 2011, 2:59 PM
 */
#include <omp.h>
#include "include/ImageWriter.hpp"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <bitset>
#include <math.h>
#include "include/messages.h"
#include "include/io.hpp"
#include "include/SkelNode.hpp"
#include "squash/squash.h"
#include "fileio/fileio.hpp"
#include "include/HuffNode.hpp"
#include "bcl/rle.h"
#include <algorithm>
#include <set>
#include <queue>
#include <boost/dynamic_bitset.hpp>
#include "include/ImageEncoder.hpp"

using namespace std;

enum IW_STATE { INVALID = -1,
                FILE_OPEN,
                HEADER_WRITTEN,
                FILE_SAVED,
                FILE_CLOSE
              } state;

string COMP_METHOD_NAME;
string ENCODING;
int COMP_LEVEL;
ofstream originalSize;


bool icompare_pred(unsigned char a, unsigned char b) {
    return std::tolower(a) == std::tolower(b);
}

bool iequals(std::string const& a, std::string const& b) {
    if (a.length() == b.length()) {
        return std::equal(b.begin(), b.end(),
                          a.begin(), icompare_pred);
    } else {
        return false;
    }
}
ImageWriter::ImageWriter(const char *fname) {
    of.open(fname, ios_base::out | ios::binary);
    of_uncompressed.open("uncompressed.sir", ios_base::out | ios::binary);

    if (of.good() && of_uncompressed.good())
    { state = FILE_OPEN; }
    else {
        PRINT(MSG_ERROR, "Could not open file for output\n");
        exit(1);
    }
    if (iequals(COMP_METHOD_NAME, "lzma")) {
        comp_mode = COMPRESS_LZMA;
    } else if (iequals(COMP_METHOD_NAME, "csc")) {
        comp_mode = COMPRESS_CSC;
    } else if (iequals(COMP_METHOD_NAME, "lzham")) {
        comp_mode = COMPRESS_LZHAM;
    } else if (iequals(COMP_METHOD_NAME, "brotli")) {
        comp_mode = COMPRESS_BROTLI;
    } else if (iequals(COMP_METHOD_NAME, "zpaq")) {
        comp_mode = COMPRESS_ZPAQ;
    } else if (iequals(COMP_METHOD_NAME, "bzip2")) {
        comp_mode = COMPRESS_BZIP2;
    } else if (iequals(COMP_METHOD_NAME, "lzma2")) {
        comp_mode = COMPRESS_LZMA2;
    } else if (iequals(COMP_METHOD_NAME, "bsc")) {
        comp_mode = COMPRESS_BSC;
    } else if (iequals(COMP_METHOD_NAME, "zlib")) {
        comp_mode = COMPRESS_ZLIB;
    } else if (iequals(COMP_METHOD_NAME, "zstd")) {
        comp_mode = COMPRESS_ZSTD;
    } else {
        PRINT(MSG_NORMAL, "Compression method not recognized. Defaulting to LZMA\n");
        comp_mode = COMPRESS_LZMA;
    }

    if (iequals(ENCODING, "huffman")) {
        encode_mode = (ENCODE_MODE::HUFFMAN);
    } else if (iequals(ENCODING, "canonical")) {
        encode_mode = (ENCODE_MODE::CANONICAL);
    } else if (iequals(ENCODING, "unitary")) {
        encode_mode = (ENCODE_MODE::UNITARY);
    } else if (iequals(ENCODING, "exp_goulomb")) {
        encode_mode = (ENCODE_MODE::EXP_GOULOMB);
    } else if (iequals(ENCODING, "arithmetic")) {
        encode_mode = (ENCODE_MODE::ARITHMETIC);
    } else if (iequals(ENCODING, "predictive")) {
        encode_mode = (ENCODE_MODE::PREDICTIVE);
    } else if (iequals(ENCODING, "compact")) {
        encode_mode = (ENCODE_MODE::COMPACT);
    } else if (iequals(ENCODING, "raw")) {
        encode_mode = (ENCODE_MODE::RAW);
    } else if (iequals(ENCODING, "mtf")) {
        encode_mode = (ENCODE_MODE::MTF);
    } else {
        encode_mode = (ENCODE_MODE::TRADITIONAL);
    }

}


/**
 * Compress the output using a compression library, and write the output to the file.
 * @return 1 when result was OK, otherwise 0.
 */

int ImageWriter::compress(ret_struct &rets) {
    PRINT(MSG_NORMAL, "Compressing output using %s...\n",
          rets.compress_algo.c_str());
    // TODO(maarten): FIX THIS SO IT ALWAYS WORKS!
    squash_set_default_search_path("../shared/squash/plugins");
    SquashCodec* codec = squash_get_codec(rets.compress_algo.c_str());
    if (codec == NULL) {
        fprintf (stderr, "Unable to find algorithm '%s'.\n",
                 rets.compress_algo.c_str());
        return EXIT_FAILURE;
    }
    size_t uncompressed_length;
    unsigned char* uncompressed;
    if (encode_mode == ENCODE_MODE::MTF) {
        uncompressed_length = ceil(rets.in_size * (257.0 / 256.0)) + 1;
        uncompressed = (unsigned char*)calloc(uncompressed_length, sizeof(unsigned char));
        const char* in_dat = (rets.in_data.c_str());
        uncompressed_length = RLE_Compress((unsigned char*)in_dat, rets.in_size, uncompressed, uncompressed_length);
        // rets.dest_len = uncompressed_length;
        // rets.out_data = uncompressed;
        PRINT(MSG_NORMAL, "Compression Done. Original size: %.5fKB, compressed size: %.5fKB.\n",
              rets.in_size / 1024.0, (uncompressed_length) / 1024.0);
    } else {

        uncompressed = (unsigned char*)rets.in_data.c_str();
        uncompressed_length = rets.in_size;
    }
    size_t compressed_length = squash_codec_get_max_compressed_size(codec, uncompressed_length);
    uint8_t* compressed = reinterpret_cast<uint8_t*>(malloc(compressed_length));
    // Compress using the highest level for smallest size
    const char* compress_level;
    if (COMP_LEVEL != -1)
        compress_level = std::to_string(COMP_LEVEL).c_str();
    else {
        switch (comp_mode) {
        case COMPRESS_LZHAM:
            compress_level = "4";
            break;
        case COMPRESS_CSC:
        case COMPRESS_ZPAQ:
            compress_level = "5";
            break;
        case COMPRESS_BROTLI:
            compress_level = "11";
            break;
        case COMPRESS_BZIP2:
        case COMPRESS_LZMA2:
        case COMPRESS_LZMA:
        case COMPRESS_ZLIB:
            compress_level = "9";
            break;

        default:
            compress_level = nullptr;
            break;
        }
        PRINT(MSG_NORMAL, "Compression level undefined. Selecting best level for method, which is %s.\n", compress_level);
    }
    SquashStatus res;
    if (compress_level == nullptr) {
        res =
            squash_codec_compress (codec,
                                   &compressed_length, compressed,
                                   uncompressed_length, (const uint8_t*) uncompressed, NULL);

    } else {
        res =
            squash_codec_compress (codec,
                                   &compressed_length, compressed,
                                   uncompressed_length, (const uint8_t*) uncompressed,
                                   // Here follows a variadic number of key/value
                                   // pairs as strings terminated by NULL
                                   // What the keys or values are is undocumented,
                                   // but at least passing the level works...
                                   "level", compress_level, NULL);
    }
    if (res != SQUASH_OK) {
        fprintf(stderr, "Unable to compress data [%d]: %s\n",
                res, squash_status_to_string (res));
        return EXIT_FAILURE;
    }

    PRINT(MSG_NORMAL, "Compression Done. Original size: %.5fKB, compressed size: %.5fKB.\n",
          rets.in_size / 1024.0, (compressed_length) / 1024.0);
    
    //fout.open("../../outputFile/compressRate.txt",ios::app); 
    //fout  << (double(rets.in_size) / double(compressed_length))<< endl;
   // fout.close();
    // rets.dest_len = uncompressed_length;
    // rets.out_data = uncompressed;
    rets.dest_len = compressed_length;
    rets.out_data = compressed;
    return res;
    free(uncompressed);
    free(compressed);
}

string ImageWriter::get_compression_name(COMPRESS_MODE mode) {
    string res = "";
    switch (mode) {
    case COMPRESS_LZMA:
        res = "xz";
        break;
    case COMPRESS_LZMA2:
        res = "lzma2";
        break;
    case COMPRESS_BROTLI:
        res = "brotli";
        break;
    case COMPRESS_ZPAQ:
        res = "zpaq";
        break;
    case COMPRESS_BSC:
        res = "bsc";
        break;
    case COMPRESS_BZIP2:
        res = "bzip2";
        break;
    case COMPRESS_ZLIB:
        res = "zlib";
        break;
    case COMPRESS_LZHAM:
        res = "lzham";
        break;
    case COMPRESS_CSC:
        res = "csc";
        break;
    case COMPRESS_ZSTD:
        res = "zstd";
        break;
    default:
        res = "lzma";
        break;
    }
    return res;
}

int ImageWriter::save() {

    ret_struct val_struct;
    ret_struct val_structCR;
    int res = 0;
    // Set up compression input
    val_struct.mode = comp_mode;
    val_struct.compress_algo = get_compression_name(comp_mode);
    PRINT(MSG_VERBOSE, "Compressing using %s\n", val_struct.compress_algo.c_str());
    val_struct.in_data = ofBuffer.str();
    val_struct.in_size = ofBuffer.str().size();

    val_structCR.mode = comp_mode;
    val_structCR.compress_algo = get_compression_name(comp_mode);
    val_structCR.in_data = ssForCR.str();
    val_structCR.in_size = ssForCR.str().size();
    // Compress the data using a predefined algorithm
    //cout<<"-------"<<ssForCR.str().size()<<endl;
    compress(val_struct);
    size_t dest_len = val_struct.dest_len;
    
  
    // Save the compressed data
    int origlen = ofBuffer.str().size();
    //int forCRsize = ssForCR.str().size();

    if (origlen > (int)dest_len) //store the data used squash
    {
        //of.write((const char *) "10000000", sizeof(uint8_t));

        unsigned char* out = val_struct.out_data;
        PRINT(MSG_NORMAL, "Squash: Bits / pixel: %3.4f\n", dest_len*8 / (double)(num_pixels));

        of.write((const char *) &comp_mode, sizeof(uint8_t));
        of.write((const char *) &origlen, sizeof(int));
        of.write((const char *) &out[0], dest_len);
        free(out);
    }
    else //Do not use squash data.
    {
        //string useSquash = "00000000";
        //unsigned char *UseSquash = reinterpret_cast<unsigned char*>(const_cast<char*>(useSquash.c_str()));
       // of.write((const char *) "00000000", sizeof(uint8_t));
        auto out = reinterpret_cast<unsigned char*>(const_cast<char*>(ofBuffer.str().c_str()));
        PRINT(MSG_NORMAL, "NoSquash: Bits / pixel: %3.4f\n", ofBuffer.str().size() / (double)(num_pixels));
        of.write((const char *) &out[0], ofBuffer.str().size());
        free(out);
    }

compress(val_structCR); //Just for calculating CR
originalSize.open("ForCR.txt",ios_base::app);
originalSize<< val_structCR.dest_len<<endl;
originalSize.close();

    state = FILE_SAVED;

    of.close();
    of_uncompressed.close();
    
    return res == SQUASH_OK;
}

void ImageWriter::writeHeader(unsigned int width, unsigned int height, int colorspace, int clear_color, int Rf, int Gf, int Bf) {
   PRINT(MSG_NORMAL, "Writing header\n");
    uint16_t out16;
    uint8_t out8;
    if (width > (2 << 16) || height > (2 << 16)) {
        PRINT(MSG_ERROR, "Due to compression the width and height of the file are limited to 65536px.\n");
        exit(1);
    }
    num_pixels = width * height;

    ofBuffer.write(reinterpret_cast<char *> (&WRITER_FILE_VERSION_NUMBER), sizeof(uint16_t));
    out16 = width;
    ofBuffer.write(reinterpret_cast<char *> (&out16), sizeof (uint16_t));
    out16 = height;
    ofBuffer.write(reinterpret_cast<char *> (&out16), sizeof (uint16_t));
    out8 = colorspace;
    ofBuffer.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
    out8 = clear_color;
    cout << "Clear color: " << clear_color << endl;
    ofBuffer.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
 
if (colorspace == COLORSPACE::RGB || colorspace == COLORSPACE::YCC || colorspace == COLORSPACE::HSV)
{
    cout<< "--"<<Rf<< "--"<<Gf<< "--"<<Bf<< "--"<<endl;
    out8 = Rf;
    ofBuffer.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
    out8 = Gf;
    ofBuffer.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
    out8 = Bf;
    ofBuffer.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
}
else
{
    out8 = Rf;
    ofBuffer.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
}
 
    state = HEADER_WRITTEN;
}

ImageWriter::~ImageWriter() {
     cout<<"here~"<<endl;
    if (state != FILE_SAVED) {
        save();
    }
   
    of.close();
    of_uncompressed.close();
}

void ImageWriter::writeBits(ostream& os, const string& str) {
    for (unsigned i = 0; i < str.length(); i += 8) {
        string sub = str.substr(i, 8);
        uint8_t bit_rep = bitset<8>(sub).to_ulong();
        os.write((const char*)(&bit_rep), sizeof(char));
    }
}

string ImageWriter::writeChain(coord3D_t prev, coord3D_t cur) {
    int16_t dx = cur.first - prev.first;
    int16_t dy = cur.second - prev.second;
    int16_t dr = cur.third - prev.third;
    // Bundling can violate this
    dr = min(dr, 2);
    dr = max(dr, -2);
    // dx_count[dx+1]++;
    // dy_count[dy+1]++;
    // dr_count[dr+2]++;
    of_uncompressed << cur.first << " - " << cur.second << " - " << cur.third << endl;
    return encoder->encode_difference(dx, dy, dr);
}

void ImageWriter::writePath(skel_tree_t *st, uint16_t pLength, bool rightMost, string& rep) {
    /* Did we reach a leaf? */
    if (st->numChildren() == 0) {
        /* if rightMost, then write END tag, we have reached end of object. */
        if (rightMost) {
            // writeBits(ofBuffer, bitset<8>(END_TAG).to_string());
            rep += encoder->encode_special_point(END_TAG, 8);
            // cout << "end" << endl;
            writeBits(ofBuffer, rep);
            encoder->reset();
            of_uncompressed << "End" << '\n';
        } else {
            rep += encoder->encode_special_point(FORK_TAG, 8);
            rep += encoder->encode_special_point(pLength, 16);
            of_uncompressed << "Fork - " << pLength << '\n';
            encoder->reset();
        }
        return;
    }

    /* Not a leaf, does it have exactly 1 child (is it a continuous path?) ?*/
    if (st->numChildren() == 1) {
        rep += writeChain(st->getValue(), st->getChild(0)->getValue());
        writePath(st->getChild(0), pLength + 1, rightMost, rep);
    }

    /* Fork coming up! */
    if (st->numChildren() > 1) {

        /* All "non-rightmost" children: */
        for (int i = 0; i < (st->numChildren() - 1) ; ++i) {
            rep += writeChain(st->getValue(), st->getChild(i)->getValue());
            writePath(st->getChild(i), 1, false, rep);
        }
        /* Treat rightmost child different, pass a longer path length, so it jumps back further after being done with the last branch. */
        rep += writeChain(st->getValue(), st->getChild( st->numChildren() - 1 )->getValue());
        writePath(st->getChild( st->numChildren() - 1 ), 1 + pLength, rightMost, rep);
    }
    return;
}



int tree_size(vector<std::pair<int, skel_tree_t*>>* forest) {
    int total_size = 0;
    vector<std::pair<int, skel_tree_t*>>::iterator pair;
    for (pair = forest->begin(); pair != forest->end(); ++pair) {
        int layer_size = 0;
        for (int i = 0; i < (pair->second)->numChildren(); ++i) {
            layer_size += (pair->second)->getChild(i)->numRChildren();
        }
        total_size += layer_size;
    }
    return total_size;
}


void ImageWriter::write_levels(vector<std::pair<int, skel_tree_t*>>* forest) {
    uint num_graylevels = 0;
    vector<uint8_t> levels;
    bool first = true;
    uint8_t prev_level = 0;
    for (auto elem = forest->begin(); elem != forest->end(); ++elem) {
        
        skel_tree_t* trees = elem->second;
        if (trees->numChildren() == 0) { continue; }
        
        if (first) {
            levels.push_back((uint8_t)elem->first);
            prev_level = (uint8_t)elem->first;
            first = false;
        } else {
            levels.push_back((uint8_t)elem->first - prev_level);
            prev_level = (uint8_t)elem->first;
        }
        num_graylevels++;
    }
//cout<<"num_graylevels "<<num_graylevels<<endl;
    ofBuffer.write(reinterpret_cast<char *>(&num_graylevels), sizeof(uint8_t));
    ofBuffer.write(reinterpret_cast<char *>(&levels[0]), num_graylevels * sizeof(uint8_t));
}
void ImageWriter::write_color_image(vector<std::pair<int, skel_tree_t*>>* red_forest, vector<std::pair<int, skel_tree_t*>>* green_forest, vector<std::pair<int, skel_tree_t*>>* blue_forest) {
    write_levels(red_forest);
    encoder = new TraditionalEncoder(red_forest);
    PRINT(MSG_VERBOSE, "Created encoder\n");

    for (auto tree_pair = red_forest->begin(); tree_pair != red_forest->end(); ++tree_pair) {
        int intensity = tree_pair->first;
        skel_tree_t* trees = tree_pair->second;
        writeLayer(trees, intensity);
    }
    if (encode_mode == ENCODE_MODE::ARITHMETIC) {
        ArithmeticEncoder* enc = (ArithmeticEncoder*)encoder;
        enc->write_arithmetic_result(ofBuffer);
    }
    delete encoder;

    write_levels(green_forest);
    encoder = new TraditionalEncoder(green_forest);
    PRINT(MSG_VERBOSE, "Created encoder\n");

    for (auto tree_pair = green_forest->begin(); tree_pair != green_forest->end(); ++tree_pair) {
        int intensity = tree_pair->first;
        skel_tree_t* trees = tree_pair->second;
        writeLayer(trees, intensity);
    }
    if (encode_mode == ENCODE_MODE::ARITHMETIC) {
        ArithmeticEncoder* enc = (ArithmeticEncoder*)encoder;
        enc->write_arithmetic_result(ofBuffer);
    }
    delete encoder;

    write_levels(blue_forest);
    encoder = new TraditionalEncoder(blue_forest);
    PRINT(MSG_VERBOSE, "Created encoder\n");

    for (auto tree_pair = blue_forest->begin(); tree_pair != blue_forest->end(); ++tree_pair) {
        int intensity = tree_pair->first;
        skel_tree_t* trees = tree_pair->second;
        writeLayer(trees, intensity);
    }
    if (encode_mode == ENCODE_MODE::ARITHMETIC) {
        ArithmeticEncoder* enc = (ArithmeticEncoder*)encoder;
        enc->write_arithmetic_result(ofBuffer);
    }
    delete red_forest;
    delete blue_forest;
    delete green_forest;

    delete encoder;
}


/* forest is a list of pairs of intensities and all corresponding paths at that intensity*/
void ImageWriter::write_image(vector<std::pair<int, skel_tree_t*>>* forest) {
    // This ought to be done in the header, but at that point we do not know
    // which layers are empty due to filtering.
    write_levels(forest);    // Set the proper encode mode for what we want to do
    // Yay polymorphism
    switch (encode_mode) {
    case (ENCODE_MODE::HUFFMAN):
        encoder = new HuffmanEncoder(forest);
        break;
    case (ENCODE_MODE::CANONICAL):
        encoder = new CanonicalHuffmanEncoder(forest);
        break;
    case (ENCODE_MODE::UNITARY):
        encoder = new UnitaryEncoder(forest);
        break;
    case (ENCODE_MODE::EXP_GOULOMB):
        encoder = new ExpGoulombEncoder(forest);
        break;
    case (ENCODE_MODE::ARITHMETIC):
        encoder = new ArithmeticEncoder(forest);
        break;
    case (ENCODE_MODE::COMPACT):
        encoder = new CompactTraditionalEncoder(forest);
        break;
    case (ENCODE_MODE::PREDICTIVE):
        encoder = new PredictiveEncoder(forest);
        break;
    case (ENCODE_MODE::TRADITIONAL):
        encoder = new TraditionalEncoder(forest);
        break;
    case (ENCODE_MODE::RAW):
        encoder = new RawEncoder(forest);
        break;
    case (ENCODE_MODE::MTF):
        encoder = new MTFEncoder(forest);
        break;
    default:
        PRINT(MSG_NORMAL, "Unspecified encoding modus, using default encoder\n");
        encoder = new TraditionalEncoder(forest);
        break;
    }
    PRINT(MSG_VERBOSE, "Created encoder\n");
    // TODO(maarten): Investigate if we can do better. Better per layer or per path?
    // Even smaller packings possible?
    // Write the whole forest.
    for (auto tree_pair = forest->begin(); tree_pair != forest->end(); ++tree_pair) {
        int intensity = tree_pair->first;
        skel_tree_t* trees = tree_pair->second;
        writeLayer(trees, intensity);
    }
    // write the result of the ArithmeticCoder to a file
    if (encode_mode == ENCODE_MODE::ARITHMETIC) {
        ArithmeticEncoder* enc = (ArithmeticEncoder*)encoder;
        enc->write_arithmetic_result(ofBuffer);
    }
    delete forest;
    delete encoder;
}

/* See file SIRFORMAT for an explanation of how the file is stored. */
void ImageWriter::writeLayer(skel_tree_t* st, unsigned char intensity) {
    
    if (state != HEADER_WRITTEN) {
        PRINT(MSG_ERROR, "Header has not been written yet -- Aborting.\n");
    }

    /* Remove overhead for empty layers. */
    if (st->numChildren() == 0) { cout<<"intensity:  "<<(int)intensity<<endl; return; }

    /* Write intensity and number of paths. */
    uint16_t num_children = (uint16_t) st->numChildren();
    of_uncompressed << "Intensity " << +intensity << " - Num children " << num_children << '\n';
    ofBuffer.write(reinterpret_cast<char *>(&num_children), sizeof(uint16_t));
    /* Top layer are disjunct paths. Treat them as separate objects */
    for (int child = 0; child < st->numChildren(); ++child) {
        skel_tree_t *curnode = (*st)[child];
        coord3D_t curpoint = curnode->getValue();
        writeBits(ofBuffer, encoder->encode_start_point(curpoint.first, curpoint.second, curpoint.third));
        of_uncompressed << curpoint.first << " - " << curpoint.second << " - " << curpoint.third << '\n';
        string rep;
        writePath(curnode, /*pLength = */1, /*rightMost = */true, rep);
    }
}

string encode_start_point(int x, int y, int r) {
    return bitset<16>(x).to_string() + bitset<16>(y).to_string() + bitset<16>(r).to_string();
}////3D
string encode_8bit_point(int x, int y, int r) {
    return bitset<8>(x).to_string() + bitset<8>(y).to_string() + bitset<8>(r).to_string();
}////3D
string encode_16bit_point(int x, int y, int r) {
    return bitset<16>(x).to_string() + bitset<16>(y).to_string() + bitset<16>(r).to_string();
}////3D

// 8-version
void ImageWriter::writeCP(unsigned int width, unsigned int height, int colorspace, int Rf) {
    ifstream ifs("controlPoint.txt"); 
    string str;
    uint8_t out8;
    int x,y,dt,imp; ////if we use uint16_t, then we cannot represent negative value.(-5 turns to 65531)
    int BranchNum, CPnum, degree;
    int last_x=0, last_y=0, last_dt=0;
    int encode_x, encode_y, encode_dt;
    
    num_pixels = width * height;

    of_uncompressed << width << " - " << height << '\n';
    writeBits(ofBuffer, bitset<16>(width).to_string() + bitset<16>(height).to_string());
    writeBits(ssForCR, bitset<16>(width).to_string() + bitset<16>(height).to_string());


    of_uncompressed << " colorspace "  << colorspace << '\n';
    writeBits(ofBuffer, bitset<8>(colorspace).to_string());
    writeBits(ssForCR, bitset<8>(colorspace).to_string());


    out8 = Rf;
    ofBuffer.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
    ssForCR.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
   

    while(ifs)//each loop is each layer
    {
        //last_x = 0; last_y = 0; last_dt = 0;
        ifs >> str;
        if(ifs.fail())  break;

        int layerNum = (int)(atof(str.c_str()));

        if(layerNum==255){
            of_uncompressed << "11111111" << '\n';
            writeBits(ofBuffer, "11111111");
            writeBits(ssForCR, "11111111");
            
            break;
        }
       
        of_uncompressed << layerNum << '\n';
        writeBits(ofBuffer, bitset<8>(layerNum).to_string());
        writeBits(ssForCR, bitset<8>(layerNum).to_string());

        ifs >> str;
        string needToInvert = bitset<8>((int)(atof(str.c_str()))).to_string();
        of_uncompressed << (int)(atof(str.c_str())) << '\n';
        writeBits(ofBuffer, needToInvert);
        writeBits(ssForCR, needToInvert);
        
    while(true)
    {
        ifs >> str;
        CPnum = (int)(atof(str.c_str()));
        if (CPnum == 0)
        {
            of_uncompressed << CPnum << '\n';
            writeBits(ofBuffer, bitset<8>(CPnum).to_string());
            writeBits(ssForCR, bitset<8>(CPnum).to_string());
            break;
        } 
        
        string firstFourBits = bitset<4>(CPnum).to_string();
       
        ifs >> str;
        degree = (int)(atof(str.c_str()));
        string lastFourBits = bitset<4>(degree).to_string();

        //of_uncompressed << firstFourBits << " - " << lastFourBits << '\n';
        of_uncompressed << CPnum << " - " << degree << '\n';
        writeBits(ofBuffer, firstFourBits + lastFourBits);
        writeBits(ssForCR, firstFourBits + lastFourBits);

        ifs >> str;
        string SampleNum = bitset<16>((int)(atof(str.c_str()))).to_string();
        of_uncompressed << (int)(atof(str.c_str())) << '\n';
        writeBits(ofBuffer, SampleNum);

        while(CPnum--)  
        {
            ifs >> str;
            x = (atof(str.c_str()));
            ifs >> str;
            y = (atof(str.c_str()));
            ifs >> str;
            dt = (atof(str.c_str()));
            //ifs >> str;////4D
            //imp = (atof(str.c_str()));
            encode_x = x - last_x;//maybe negive value.
            encode_y = y - last_y;//maybe negive value.
            encode_dt = dt - last_dt;//maybe negive value.
            if (encode_x == 0 && encode_y == 0) {encode_x = 1;cout<<"attention the error!"<<endl;}//To avoid conflict with the flag!!
            if (encode_x > 127 || encode_y > 127 || encode_dt > 127 || encode_x < -127 || encode_y < -127 || encode_dt < -127)
            {
                of_uncompressed << "0000000000000000" << encode_x << " / " << encode_y << " / " << encode_dt << '\n';
                writeBits(ofBuffer, "0000000000000000");
                writeBits(ofBuffer, encode_16bit_point(encode_x,encode_y,encode_dt));
                writeBits(ssForCR, "0000000000000000");
                writeBits(ssForCR, encode_16bit_point(encode_x,encode_y,encode_dt));
            }
            else
            {
                of_uncompressed << encode_x << " / " << encode_y << " / " << encode_dt << '\n';
                writeBits(ofBuffer, encode_8bit_point(encode_x,encode_y,encode_dt));
                writeBits(ssForCR, encode_8bit_point(encode_x,encode_y,encode_dt));
            }
            last_x = x;
            last_y = y;
            last_dt = dt;
        } 
     }
    }  
}

void ImageWriter::writeColorCP(unsigned int width, unsigned int height, int colorspace, int Rf, int Gf, int Bf) {
    int totalCPs, totalBranchNum = 0;
    ifstream ifs("controlPoint.txt"); 
    string str;
    uint8_t out8;
    int x,y,dt,imp; ////if we use uint16_t, then we cannot represent negative value.(-5 turns to 65531)
    int BranchNum, CPnum, degree;
    int last_x=0, last_y=0, last_dt=0;
    int encode_x, encode_y, encode_dt;
    
    num_pixels = width * height;

    of_uncompressed << width << " - " << height << '\n';
    writeBits(ofBuffer, bitset<16>(width).to_string() + bitset<16>(height).to_string());
    writeBits(ssForCR, bitset<16>(width).to_string() + bitset<16>(height).to_string());

    of_uncompressed << " colorspace "  << colorspace << '\n';
    writeBits(ofBuffer, bitset<8>(colorspace).to_string());
    writeBits(ssForCR, bitset<8>(colorspace).to_string());


    out8 = Rf;
    ofBuffer.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
    ssForCR.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
    out8 = Gf;
    ofBuffer.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
    ssForCR.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
    out8 = Bf;
    ofBuffer.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));
    ssForCR.write(reinterpret_cast<char *> (&out8), sizeof (uint8_t));


    while(ifs)//each loop is for each layer
    {
        //last_x = 0; last_y = 0; last_dt = 0;
        ifs >> str;
        if(ifs.fail())  break;

        int layerNum = (int)(atof(str.c_str()));

        if(layerNum==255){
            of_uncompressed << "11111111" << '\n';
            writeBits(ofBuffer, "11111111");
            writeBits(ssForCR, "11111111");
            
            ifs >> str;
            layerNum = (int)(atof(str.c_str()));
            cout<<"layerNum "<<layerNum<<endl;
            if(ifs.fail())  break;
            if(layerNum==255)  {writeBits(ofBuffer, "11111111"); break;} //in case G and B channels are empty.

        }
       
        of_uncompressed << layerNum << '\n';
        writeBits(ofBuffer, bitset<8>(layerNum).to_string());
        writeBits(ssForCR, bitset<8>(layerNum).to_string());
    

        ifs >> str;
        string needToInvert = bitset<8>((int)(atof(str.c_str()))).to_string();
        of_uncompressed << (int)(atof(str.c_str())) << '\n';
        writeBits(ofBuffer, needToInvert);
        writeBits(ssForCR, needToInvert);
        
    while(true) //for each branch.
    {
        totalBranchNum ++;
        ifs >> str;
        CPnum = (int)(atof(str.c_str()));
        if (CPnum == 0)
        {
            of_uncompressed << CPnum << '\n';
            writeBits(ofBuffer, bitset<8>(CPnum).to_string());
            writeBits(ssForCR, bitset<8>(CPnum).to_string());
            break;
        } 
        
        string firstFourBits = bitset<4>(CPnum).to_string();
        totalCPs += CPnum;

        ifs >> str;
        degree = (int)(atof(str.c_str()));
        string lastFourBits = bitset<4>(degree).to_string();

        //of_uncompressed << firstFourBits << " - " << lastFourBits << '\n';
        of_uncompressed << CPnum << " - " << degree << '\n';
        writeBits(ofBuffer, firstFourBits + lastFourBits);
        writeBits(ssForCR, firstFourBits + lastFourBits);

        ifs >> str;
        string SampleNum = bitset<16>((int)(atof(str.c_str()))).to_string();
        of_uncompressed << (int)(atof(str.c_str())) << '\n';
        writeBits(ofBuffer, SampleNum);

        while(CPnum--)  
        {
            ifs >> str;
            x = (atof(str.c_str()));
            ifs >> str;
            y = (atof(str.c_str()));
            ifs >> str;
            dt = (atof(str.c_str()));
            //ifs >> str;////4D
            //imp = (atof(str.c_str()));
            encode_x = x - last_x;//maybe negative value.
            encode_y = y - last_y;//maybe negative value.
            encode_dt = dt - last_dt;//maybe negative value.
            if (encode_x == 0 && encode_y == 0) {encode_x = 1;cout<<"attention the error!"<<endl;}//To avoid conflict with the flag!!
            if (encode_x > 127 || encode_y > 127 || encode_dt > 127 || encode_x < -127 || encode_y < -127 || encode_dt < -127)
            {
                of_uncompressed << "0000000000000000" << encode_x << " / " << encode_y << " / " << encode_dt << '\n';
                writeBits(ofBuffer, "0000000000000000");
                writeBits(ofBuffer, encode_16bit_point(encode_x,encode_y,encode_dt));
                writeBits(ssForCR, "0000000000000000");
                writeBits(ssForCR, encode_16bit_point(encode_x,encode_y,encode_dt));
            }
            else
            {
                of_uncompressed << encode_x << " / " << encode_y << " / " << encode_dt << '\n';
                writeBits(ofBuffer, encode_8bit_point(encode_x,encode_y,encode_dt));
                writeBits(ssForCR, encode_8bit_point(encode_x,encode_y,encode_dt));
            }
            last_x = x;
            last_y = y;
            last_dt = dt;
        } 
     }
    }   
   
    of_uncompressed << " %%%%%%%% totalBranchNum: "  << totalBranchNum << " totalCPs: " << totalCPs << '\n';
}