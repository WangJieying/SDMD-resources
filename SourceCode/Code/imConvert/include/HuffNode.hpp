/*
 * File:   HuffNode.hpp
 * Author: Yuri Meiburg <yuri@meiburg.nl>
 *
 * Created on July 7, 2011, 6:00 PM
 */

#ifndef HUFFNODE_HPP
#define HUFFNODE_HPP
using namespace std;
template< class T, class Q > class HuffNode {
  private:
    T value;
    Q frequency;
    HuffNode<T, Q>* left;
    HuffNode<T, Q>* right;

  public:
    HuffNode(T v, Q freq): value(v), frequency(freq) {}; //{ value = v; } ;

    T getValue() const { return value; }
    Q getFrequency() const { return frequency; }
    bool is_leaf() const { return left == NULL && right == NULL;}
    HuffNode<T, Q>* get_left_child() const {return left;}
    HuffNode<T, Q>* get_right_child() const {return right;}

    void set_left_child(HuffNode<T, Q>* child) {left = child;}
    void set_right_child(HuffNode<T, Q>* child) {right = child;}
};

#endif  /* HUFFNODE_HPP */

