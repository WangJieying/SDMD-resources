#include "include/ImageEncoder.hpp"
#include <iostream>
#include "fileio/fileio.hpp"
using namespace std;

void ImageEncoder::add_to_hist(coord3D_t cur, coord3D_t prev) {
    int dx = cur.first - prev.first;
    int dy = cur.second - prev.second;
    int dr = cur.third - prev.third;
 
    // if (prev_dx != -1000 && prev_dy != -1000 && prev_dr != -1000) {
    //     int prev_idx = distance(point_list.begin(), point_list.find(make_tuple(prev_dx, prev_dy, prev_dr)));
    //     int curr_idx = distance(point_list.begin(), point_list.find(make_tuple(dx, dy, dr)));
    //     predictive_hist[prev_idx][curr_idx]++;
    // }
    int curr_idx = distance(point_list.begin(), point_list.find(make_tuple(dx, dy, dr)));
    point_list[make_tuple(dx, dy, dr)]++;
    MIN_DX = min(dx, MIN_DX);
    MIN_DY = min(dy, MIN_DY);
    MIN_DR = min(dr, MIN_DR);
    MAX_DX = max(dx, MAX_DX);
    MAX_DY = max(dy, MAX_DY);
    MAX_DR = max(dr, MAX_DR);
    // floor(log2(x) + 1) is the minimal number of bits needed to represent x
    // So we take the minimal number of bits necessary, ceiled to the nearest byte with a minimum of one byte
    BITS_BIG_DX = max(8, max(round_next_pow_2(floor(log2(MAX_DX + abs(MIN_DX)) + 1)), BITS_BIG_DX));
    BITS_BIG_DY = max(8, max(round_next_pow_2(floor(log2(MAX_DY + abs(MIN_DY)) + 1)), BITS_BIG_DY));
    BITS_BIG_DR = max(8, max(round_next_pow_2(floor(log2(MAX_DR + abs(MIN_DR)) + 1)), BITS_BIG_DR));
    // histogram[dx]++;
    // histogram[dy]++;
    // histogram[dr]++;
    histogram[curr_idx]++;
    prev_dx = dx;
    prev_dy = dy;
    prev_dr = dr;
}



void ImageEncoder::traverse_path(skel_tree_t *st, uint16_t pLength, bool rightMost) {
    /* Not a leaf, does it have exactly 1 child (is it a continuous path?) ?*/
    if (st->numChildren() == 0) {
        /* if rightMost, then write END tag, we have reached end of object. */
        if (rightMost) {
            // histogram[END_TAG]++;
            prev_dx = -1000; prev_dy = -1000; prev_dr = -1000;
        } else {
            // histogram[FORK_TAG]++;
            // histogram[pLength]++;
        }
        return;
    }

    if (st->numChildren() == 1) {
        add_to_hist(st->getValue(), st->getChild(0)->getValue());
        traverse_path(st->getChild(0), pLength + 1, rightMost);
        return;
    }

    /* Fork coming up! */
    if (st->numChildren() > 1) {
        /* All "non-rightmost" children: */
        auto cur = st->getValue();
        for (int i = 0; i < (st->numChildren() - 1) ; ++i) {
            auto prev = st->getChild(i)->getValue();
            add_to_hist(cur, prev);
            traverse_path(st->getChild(i), 1, false);
        }
        /* Treat rightmost child different, pass a longer path length,
        so it jumps back further after being done with the last branch. */
        add_to_hist(cur, st->getChild(st->numChildren() - 1)->getValue());
        traverse_path(st->getChild( st->numChildren() - 1 ), 1 + pLength, rightMost);
    }
}

void ImageEncoder::populate_encoding_histogram(skel_tree_t* st) {
    /* Remove overhead for empty layers. */
    if (st->numChildren() == 0) { return; }

    /* Top layer are disjunct paths. Treat them as separate objects */
    for (int child = 0; child < st->numChildren(); ++child) {
        skel_tree_t *curnode = (*st)[child];
        traverse_path(curnode, /*pLength = */1, /*rightMost = */true);
    }
}


map<int, BitSet> HuffmanEncoder::huffman_encode(std::map<int, int> &histogram) {
    // Comparison of HuffNodes
    struct DereferenceCompareNode : public std::binary_function<HuffNode<int, int>*, HuffNode<int, int>*, bool> {
        bool operator()(const HuffNode<int, int>* lhs, const HuffNode<int, int>* rhs) const {
            return lhs->getFrequency() > rhs->getFrequency();
        }
    };

    std::priority_queue<HuffNode<int, int>*, std::vector<HuffNode<int, int>*>, DereferenceCompareNode> queue;
    // since it's reverse sorted, start with the last element
    for (auto elem : histogram) {
        int message = elem.first;
        int frequency = elem.second;
        if (frequency != 0) {
            HuffNode<int, int>* n = new HuffNode<int, int>(message, frequency);
            n->set_left_child(NULL);
            n->set_right_child(NULL);
            queue.push(n);
        }
    }
    while (queue.size() > 1) {
        auto elem_1 = queue.top();
        queue.pop();
        auto elem_2 = queue.top();
        queue.pop();
        HuffNode<int, int>* new_elem = new HuffNode<int, int>(0, elem_1->getFrequency() + elem_2->getFrequency());
        new_elem->set_left_child(elem_1);
        new_elem->set_right_child(elem_2);
        queue.push(new_elem);
    }

    map<int, BitSet> res_map;
    BitSet prefix(0);
    cout << queue.top()->getFrequency() << endl;
    create_encodings(queue.top(), prefix, res_map);
    for (auto elem : res_map) {
        auto point = points[elem.first];
        int count = point_list[point];
        cout << elem.first << ", (" << get<0>(point) << ", " << get<1>(point) << ", " << get<2>(point) << "), " << count << ": " << elem.second << endl;
    }
    return res_map;
}

string HuffmanEncoder::encode_difference(int dx, int dy, int dr) {
    // string encoded_dx;
    // string encoded_dy;
    // string encoded_dr;
    // to_string(huffman_coding[dx], encoded_dx);
    // to_string(huffman_coding[dy], encoded_dy);
    // to_string(huffman_coding[dr], encoded_dr);
    // return (encoded_dx) + (encoded_dy) + (encoded_dr);
    // num_symbols++;
    int curr_idx = distance(point_list.begin(), point_list.find(make_tuple(dx, dy, dr)));
    string enc;
    to_string(huffman_coding[curr_idx], enc);
    return enc;

}

bool code_word_sort(pair<int, BitSet> left, pair<int, BitSet> right) {
    if (left.second.size() < right.second.size())
        return true;
    if (left.second.size() > right.second.size())
        return false;
    return left.second.to_ulong() < right.second.to_ulong();
}

void CanonicalHuffmanEncoder::canonicalize() {
    vector<pair<int, BitSet>> elems;
    for (auto elem : huffman_coding)
        elems.push_back(elem);
    sort(elems.begin(), elems.end(), code_word_sort);
    uint code = 0;
    BitSet first_code = BitSet(elems[0].second.size(), 0);
    huffman_coding[elems[0].first] = first_code;
    for (uint i = 1; i < elems.size(); ++i) {
        code = (code + 1) << (elems[i].second.size() - elems[i - 1].second.size());
        huffman_coding[elems[i].first] = BitSet(elems[i].second.size(), code);
    }

}
