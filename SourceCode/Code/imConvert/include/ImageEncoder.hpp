#ifndef IMAGE_ENCODER_GUARD
#define IMAGE_ENCODER_GUARD

#include <map>
#include <vector>
#include "Triple.hpp"
#include "SkelNode.hpp"
#include "HuffNode.hpp"
#include "../fileio/fileio.hpp"
#include "messages.h"
#include "../shared/FastAC/arithmetic_codec.h"
#include <sstream>
#include <math.h>
#include <iostream>
#include <numeric>
#include <queue>
#include <boost/dynamic_bitset.hpp>
#include <bitset>
typedef Triple<int, int, int> coord3D_t;
typedef SkelNode< coord3D_t > skel_tree_t;
typedef boost::dynamic_bitset<uint8_t> BitSet;

class ImageEncoder {
public:
    ImageEncoder() : ImageEncoder(nullptr) {
        std::cout << "Base ctor" << endl;
    };
    ImageEncoder(vector<std::pair<int, skel_tree_t*>>* forest) {
        int i = 0;
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                if (x == 0 && y == 0) continue;
                for (int r = -2; r <= 2; ++r) {
                    // printf("%d: %d - %d - %d\n", i++, x, y, r);
                    point_list[make_tuple(x, y, r)] = 0;
                    points[i++] = make_tuple(x, y, r);
                    // predictive_hist[i++] = (int*)(calloc(40, sizeof(int)));
                }
            }
        }
        // for (int i = 0; i <= 4; ++i) {
        //     for (int j = 0; j <= 4; ++j) {
        //         for (int q = 0; q <= 8; ++q)
        //             predictor_enc_map.push_back(make_tuple(i, j, q));
        //     }
        // }
        for (auto elem = forest->begin(); elem != forest->end(); ++elem) {
            skel_tree_t* trees = elem->second;
            populate_encoding_histogram(trees);
        }
        // set up the predictor
        // int j = 0;

        // for (auto elem : predictive_hist) {
        //     // cout << elem.first << ": ";
        //     vector<std::pair<int, int>> hist;
        //     for (int i = 0; i < 40; ++i)
        //         hist.push_back(make_pair(elem.second[i], i));
        //     std::sort(hist.begin(), hist.end(),     [](const std::pair<int, int> & a, const std::pair<int, int> & b) -> bool {
        //         return a.first > b.first;
        //     });
        //     printf("%d: %d - %d\n", j, hist.front().first, hist.front().second);
        //     predictor[j++] = hist.front().second;
        // }
    } 
    virtual ~ImageEncoder() {};
    virtual void reset() {free(prev_point); prev_point = nullptr;};
    virtual string encode_difference(int dx, int dy, int dr) = 0;
    virtual string encode_start_point(uint16_t x, uint16_t y, uint16_t r) {
        return bitset<16>(x).to_string() + bitset<16>(y).to_string() + bitset<16>(r).to_string();
    }
    virtual string encode_special_point(int point, int num_bits) {
        if (num_bits == 8)
            return bitset<8>(point).to_string();
        else
            return bitset<16>(point).to_string();
        prev_point = nullptr;
    }
    void populate_encoding_histogram(skel_tree_t* st);
    int BITS_BIG_DX = -1000, BITS_BIG_DY = -1000, BITS_BIG_DR = -1000;
    int MIN_DX = 1000, MIN_DY = 1000, MIN_DR = 1000;
    int MAX_DX = -1000, MAX_DY = -1000, MAX_DR = -1000;
    // This is a map from an index in point list to a list of indices in point list
    // It thus contains a point mapping to a histogram of points that come after that
    // int predictor[40] = {};
    std::map<int, int*> predictive_hist;
    // int num_symbols = 0;
    map<pair<int, int>, int> subseq_pair_count;
protected:
    int* prev_point = nullptr;
    enum ENCODE_MODE {HUFFMAN = 1, CANONICAL, UNITARY, EXP_GOULOMB, ARITHMETIC, TRADITIONAL};

    void add_to_hist(coord3D_t cur, coord3D_t prev);
    void traverse_path(skel_tree_t *st, uint16_t pLength, bool rightMost);

    int prev_dx = -1000;
    int prev_dy = -1000;
    int prev_dr = -1000;
    std::vector<tuple<int, int, int>> predictor_enc_map;
    std::ostringstream oss;
    std::map<int, int> histogram;
    std::tuple<int, int, int> points[40];
    std::map<std::tuple<int, int, int>, int> point_list;
    int round_next_pow_2(double d) {
        int v = round(d);
        (v)--;
        (v) |= (v) >> 1;
        (v) |= (v) >> 2;
        (v) |= (v) >> 4;
        (v) |= (v) >> 8;
        (v) |= (v) >> 16;
        (v)++;
        return v;
    }
};


class CompactTraditionalEncoder : public ImageEncoder {
public:
    CompactTraditionalEncoder(vector<std::pair<int, skel_tree_t*>>* forest) : ImageEncoder(forest) {}
    string encode_difference(int dx, int dy, int dr) {
        uint8_t i;
        for (i = 0; i < 40; ++i) {
            auto point = points[i];
            if (get<0>(point) == dx &&
                    get<1>(point) == dy &&
                    get<2>(point) == dr)
                break;
        }
        return bitset<6>(i).to_string();
    }
};
class MTFEncoder : public ImageEncoder {
public:
    MTFEncoder(vector<std::pair<int, skel_tree_t*>>* forest) : ImageEncoder(forest) {
        cout << "MTF" << endl;
        // for (int i = 0; i < 40; ++i)
        //     mtf[i] = i;
    }
    string encode_difference(int dx, int dy, int dr) {
        // int pos_p = 0;
        // for(; pos_p < 40; ++pos_p)
        // {
        //     if(get<0>(points[pos_p]) == dx &&
        //        get<1>(points[pos_p]) == dy &&
        //        get<2>(points[pos_p]) == dr)
        //         break;
        // }
        // int to_ret = 0;
        // for(; to_ret < 40; ++to_ret)
        //     if(to_ret == mtf[pos_p])
        //         break;

        // cout << to_ret << endl;
        // move_to_front(to_ret);
        // return bitset<8>(to_ret).to_string();
        int pos_dx, pos_dy, pos_dr;
        // DX PART
        for (pos_dx = 0; pos_dx < 3; ++pos_dx)
            if (dx == dx_mtf[pos_dx])
                break;

        move_to_front(pos_dx, dx_mtf);
        // DY PART
        for (pos_dy = 0; pos_dy < 3; ++pos_dy)
            if (dy == dy_mtf[pos_dy])
                break;

        move_to_front(pos_dy, dy_mtf);
        // DR PART
        for (pos_dr = 0; pos_dr < 5; ++pos_dr)
            if (dr == dr_mtf[pos_dr])
                break;

        move_to_front(pos_dr, dr_mtf);
        return bitset<2>(pos_dx).to_string() + bitset<2>(pos_dy).to_string() + bitset<3>(pos_dr).to_string();
    }
private:
    int dx_mtf[3] = { -1, 0, 1};
    int dy_mtf[3] = { -1, 0, 1};
    int dr_mtf[5] = { -2, -1, 0, 1, 2};
    // int mtf[40];
    void move_to_front(int i, int* mtf) {
        int t = mtf[i];
        for ( int z = i - 1; z >= 0; z-- )
            mtf[z + 1] = mtf[z];

        mtf[0] = t;
    }
};
class PredictiveEncoder: public ImageEncoder {
public:
    PredictiveEncoder(vector<std::pair<int, skel_tree_t*>>* forest) : ImageEncoder(forest) {
        std::cout << "predictor" << endl;
        prev_dx = -1000;
        prev_dy = -1000;
        prev_dr = -1000;
        for (int ddx = -1; ddx <= -1; ++ddx) {
            for (int ddy = -1; ddy <= -1; ++ddy) {
                for (int ddr = -1; ddr <= -1; ++ddr) {
                    predictor.push_back(make_tuple(ddx, ddy, ddr));
                }
            }
        }

    }
    void reset() {
        prev_dx = -1000;
        prev_dy = -1000;
        prev_dr = -1000;
    }
    vector<tuple<int, int, int>> predictor;

    string encode_difference(int dx, int dy, int dr) {
        string encoded_point = "";
        // uint8_t num = point_list.size();
        auto p = make_tuple(prev_dx - dx, prev_dy - dy, prev_dr - dr);
        auto location = find(predictor.begin(), predictor.end(), p);
        if (location == predictor.end()) {
            encoded_point = bitset<2>(dx).to_string() + bitset<2>(dy).to_string() + bitset<3>(dr).to_string();
        } else {
            encoded_point = bitset<8>(64 + std::distance(predictor.begin(), location)).to_string();
        }

        // if (prev_dx == -1000 && prev_dy == -1000 && prev_dr == -1000) {
        //     encoded_point.append("0");
        //     auto encoded_dx = bitset<2>(dx + 1).to_string();
        //     auto encoded_dy = bitset<2>(dy + 1).to_string();
        //     auto encoded_dr = bitset<3>(dr + 2).to_string();
        //     encoded_point.append(encoded_dx + encoded_dy + encoded_dr);
        // } else {
        //     auto p = make_pair(make_tuple(prev_dx, prev_dy, prev_dr), make_tuple(dx, dy, dr));
        //     auto location = find(predictor.begin(), predictor.end(), p);
        //     if(location == predictor.end())
        //     {
        //         predictor.push_back(p);
        //         encoded_point = bitset<8>(40 + predictor.size() - 1).to_string();
        //     }
        //     else
        //     {
        //         encoded_point = bitset<8>(40 + std::distance(predictor.begin(), location)).to_string();
        //     }
        // Find which point ought to be most likely given the previous point
        // int prev_idx = FindIndex(points, 40, make_tuple(prev_dx, prev_dy, prev_dr));
        // auto most_likely = points[(predictor[prev_idx])];
        // // Find the differences
        // int8_t pred_dx = get<0>(most_likely) - dx;
        // int8_t pred_dy = get<1>(most_likely) - dy;
        // int8_t pred_dr = get<2>(most_likely) - dr;
        // // Try to encode it
        // uint8_t m_pred_dx = pred_dx < 0 ? 2 - pred_dx : pred_dx;
        // uint8_t m_pred_dy = pred_dy < 0 ? 2 - pred_dy : pred_dy;
        // uint8_t m_pred_dr = pred_dr < 0 ? 4 - pred_dr : pred_dr;

        // uint8_t i;
        // for (i = 0; i < predictor_enc_map.size(); ++i) {
        //     if (get<0>(predictor_enc_map.at(i)) == m_pred_dx &&
        //             get<1>(predictor_enc_map.at(i)) == m_pred_dy &&
        //             get<2>(predictor_enc_map.at(i)) == m_pred_dr)
        //         break;
        // }
        // encoded_point = bitset<8>(i).to_string();

        prev_dx = dx;
        prev_dy = dy;
        prev_dr = dr;
        return encoded_point;
    }
protected:
    size_t FindIndex( const tuple<int, int, int> a[], size_t size, tuple<int, int, int> value ) {
        size_t index = 0;

        while ( index < size && a[index] != value ) ++index;

        return ( index == size ? -1 : index );
    }
    int prev_dx;
    int prev_dy;
    int prev_dr;

};
class HuffmanEncoder: public ImageEncoder {
public:
    HuffmanEncoder(vector<std::pair<int, skel_tree_t*>>* forest) : ImageEncoder(forest) {
        std::cout << "Huffman" << endl;

        huffman_coding = huffman_encode(histogram);
    };
    string encode_difference(int dx, int dy, int dr);
protected:
    map<int, int> a_histogram;
    map<int, BitSet> huffman_encode(std::map<int, int> &histogram);
    map<int, BitSet> huffman_coding;
    void create_encodings(HuffNode<int, int>* tree, BitSet prefix, map<int, BitSet>& res_map) {
        if (tree->is_leaf()) {
            int val = tree->getValue();
            res_map[val] = prefix;
        } else {
            auto left_prefix = prefix;
            left_prefix.push_back(0);
            create_encodings(tree->get_left_child(), left_prefix, res_map);

            auto right_prefix = prefix;
            right_prefix.push_back(1);
            create_encodings(tree->get_right_child(), right_prefix, res_map);
        }
    }

};

class CanonicalHuffmanEncoder: public HuffmanEncoder {
public:
    CanonicalHuffmanEncoder(vector<std::pair<int, skel_tree_t*>>* forest) : HuffmanEncoder(forest) {
        std::cout << "Canonical" << endl;

        canonicalize();
    };
    using HuffmanEncoder::encode_difference;
protected:
    std::map<int, int> histogram;
    void canonicalize();
};

class ArithmeticEncoder: public ImageEncoder {
private:
    Arithmetic_Codec ace;
    Adaptive_Data_Model point_model;
    unsigned char* buf;
public:
    ArithmeticEncoder(vector<std::pair<int, skel_tree_t*>>* forest) : ImageEncoder(forest) {
        std::cout << "Arithmetic" << endl;

        int point_sum = std::accumulate(std::begin(point_list), std::end(point_list), 0, [](const int previous, const std::pair<tuple<int, int, int>, int>& p) { return previous + p.second; });
        double* probs = new double[point_list.size()];
        int i = 0;
        // if(typeid(point_model) == typeid(Static_Data_Model))
        // {
        cout << "here" << endl;
        for (auto it = point_list.begin(); it != point_list.end(); ++it) {
            auto point = it->first;
            cout << "(" << get<0>(point) << ", " << get<1>(point) << ", " << get<2>(point) << ")" << endl;
            probs[i++] = it->second / (double)point_sum;
        }
        // }
        // This is necessary because...?
        buf = nullptr;
        ace.set_buffer(1e7, buf);
        // Use this one with a adaptive data model
        point_model.set_alphabet(point_list.size() + 2);
        // Use this one with a static data model
        // point_model.set_distribution(point_list.size(), probs);
        ace.start_encoder();

    };
    void write_arithmetic_result(stringstream& ofBuffer) {
        cout << "Called destructor" << endl;
        int num_bytes = ace.stop_encoder();
        buf = ace.buffer();
        // oss << std::string((const char*)buf, num_bytes);
        ofBuffer.write(reinterpret_cast<char *>(buf), num_bytes * sizeof(unsigned char));
    }
    string encode_difference(int dx, int dy, int dr) {
        int idx = distance(point_list.begin(), point_list.find(make_tuple(dx, dy, dr)));
        // cout << idx << endl;
        ace.encode(idx, point_model);
        return "";
    }
    string encode_start_point(uint16_t x, uint16_t y, uint16_t r) {
        ace.put_bits(x, 16);
        ace.put_bits(y, 16);
        ace.put_bits(r, 16);
        return "";
    }
    string encode_special_point(int point, int num_bits) {
        if (point == END_TAG)
            ace.encode(point_list.size(), point_model);
        else if (point == FORK_TAG)
            ace.encode(point_list.size() + 1, point_model);
        else
            ace.put_bits(point, num_bits);
        return "";
    }


};
class RawEncoder : public ImageEncoder {
public:
    RawEncoder(vector<std::pair<int, skel_tree_t*>>* forest) : ImageEncoder(forest) {
        cout << "Raw" << endl;
    }
    string encode_difference(int dx, int dy, int dr) {
        string dx_string, dy_string, dr_string;
        if (dx == -1)
            dx_string = "0";
        else if (dx == 0)
            dx_string = "10";
        else
            dx_string = "11";

        if (dy == -1)
            dy_string = "10";
        else if (dy == 0)
            dy_string = "11";
        else
            dy_string = "0";

        if (dr == -2)
            dr_string = "1001";
        else if (dr == -1)
            dr_string = "11";
        else if (dr == 0)
            dr_string = "0";
        else if (dr == 1)
            dr_string = "101";
        else
            dr_string = "1000";
        return dx_string + dy_string + dr_string;
    }
};
class TraditionalEncoder: public ImageEncoder {
public:
    TraditionalEncoder(vector<std::pair<int, skel_tree_t*>>* forest) : ImageEncoder(forest) {
        std::cout << "Traditional" << endl;

    };

    string encode_difference(int dx, int dy, int dr) {
        string encoded_point;
        // if(prev_point != nullptr)
        // {
        //     int curr_idx = distance(point_list.begin(), point_list.find(make_tuple(dx, dy, dr)));
        //     subseq_pair_count[make_pair(*prev_point, curr_idx)]++;
        //     *prev_point = curr_idx;
        // }
        // else
        // {
        //     int curr_idx = distance(point_list.begin(), point_list.find(make_tuple(dx, dy, dr)));
        //     prev_point = (int*)malloc(sizeof(int));
        //     *prev_point = curr_idx;
        // }

        bool small_point = dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1 && dr >= -2 && dr <= 2;
        if (small_point) {
            // PRINT(MSG_NORMAL, "Small point\n");
            // Small encoding
            encoded_point.append("0");
            auto encoded_dx = bitset<2>(dx + 1).to_string();
            auto encoded_dy = bitset<2>(dy + 1).to_string();
            auto encoded_dr = bitset<3>(dr + 2).to_string();
            encoded_point.append(encoded_dx + encoded_dy + encoded_dr);
            return encoded_point;
        } else {
            // cout << dx << ", " << dy << ", " << dr << endl;
            // PRINT(MSG_NORMAL, "Big point\n");
            bool small_big_point = abs(dx) <= 15 && abs(dy) <= 15 && abs(dr) <= 15 && dx != -15;
            if (small_big_point) {
                uint16_t num = (1 << 15);
                num += (dr + 15);
                num += ((dy + 15) << 5);
                num += ((dx + 15) << 10);
                return (bitset<16>(num).to_string());
            } else {
                // Big point, does not fit
                string encoded_dx, encoded_dy, encoded_dr;
                string encoded_point = (bitset<8>(128).to_string());
                to_string(boost::dynamic_bitset<>(BITS_BIG_DX, dx), encoded_dx);
                to_string(boost::dynamic_bitset<>(BITS_BIG_DY, dy), encoded_dy);
                to_string(boost::dynamic_bitset<>(BITS_BIG_DR, dr), encoded_dr);
                return encoded_point + (encoded_dx) + (encoded_dy) + (encoded_dr);
            }
        }
    }

};
class UnitaryEncoder: public ImageEncoder {
public:
    UnitaryEncoder(vector<std::pair<int, skel_tree_t*>>* forest) : ImageEncoder(forest) {};

    string encode_difference(int dx, int dy, int dr) {
        uint16_t mapped_dx = dx <= 0 ? -2 * dx : 2 * dx - 1;
        uint16_t mapped_dy = dy <= 0 ? -2 * dy : 2 * dy - 1;
        uint16_t mapped_dr = dr <= 0 ? -2 * dr : 2 * dr - 1;
        string encoded_dx, encoded_dy, encoded_dr;
        to_string(unitary_encode(mapped_dx), encoded_dx);
        to_string(unitary_encode(mapped_dy), encoded_dy);
        to_string(unitary_encode(mapped_dr), encoded_dr);
        return (encoded_dx) + (encoded_dy) + (encoded_dr);

    }
private:
    boost::dynamic_bitset<> unitary_encode(uint16_t n) {
        boost::dynamic_bitset<> res(n + 1, 0);
        res.flip();
        res.reset(0);
        return res;
    }

};
class ExpGoulombEncoder: public ImageEncoder {
public:
    ExpGoulombEncoder(vector<std::pair<int, skel_tree_t*>>* forest) : ImageEncoder(forest) {};

    string encode_difference(int dx, int dy, int dr) {
        uint16_t mapped_dx = dx <= 0 ? -2 * dx : 2 * dx - 1;
        uint16_t mapped_dy = dy <= 0 ? -2 * dy : 2 * dy - 1;
        uint16_t mapped_dr = dr <= 0 ? -2 * dr : 2 * dr - 1;
        string encoded_dx, encoded_dy, encoded_dr;
        to_string(goulomb_encode(mapped_dx), encoded_dx);
        to_string(goulomb_encode(mapped_dy), encoded_dy);
        to_string(goulomb_encode(mapped_dr), encoded_dr);
        return (encoded_dx) + (encoded_dy) + (encoded_dr);

    }
private:
    boost::dynamic_bitset<> goulomb_encode(uint16_t n) {
        int num_zeros = 31 - __builtin_clz((unsigned int)(n + 1));
        boost::dynamic_bitset<> res(2 * num_zeros + 1, n + 1);
        return res;
    }
};
#endif