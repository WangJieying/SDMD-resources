/* 
 * File:   Triple.hpp
 * Author: yuri
 *
 * Created on June 8, 2011, 11:09 AM
 */

#ifndef TRIPLE_HPP
#define	TRIPLE_HPP

template <class T, class U, class V> class Triple {
public:
    T first;
    U second;
    V third;
    Triple(T first, U second, V third){
        this->first = first;
        this->second=second;
        this->third = third;
    };
    //Triple(){
    //    cout << "[Triple] Warning, not initializing memory." << endl;
    //}
};

#endif	/* TRIPLE_HPP */

