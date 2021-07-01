/*
 * File:   Triple.hpp
 * Author: yuri
 *
 * Created on June 8, 2011, 11:09 AM
 */

#ifndef TRIPLE_HPP
#define TRIPLE_HPP

#include<iostream>
using namespace std;

template <class T, class U, class V> class Triple
{
    public:
        T first;
        U second;
        V third;
        Triple(T first, U second, V third)
        {
            this->first = first;
            this->second = second;
            this->third = third;
        };

        /* Inline this friend function will ensure that the compiler generates the correct functions :) */
        friend ostream &operator<< (ostream &os, const Triple &tr)
        {
            os << "{" << tr.first << ", " << tr.second << ", " << tr.third << "}";
            return os;
        }
};




#endif  /* TRIPLE_HPP */