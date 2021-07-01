#include<iostream>
#include <fstream>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
using namespace std;

template <class T> T * readFile(const char *fn){
    char* data;
    ifstream file(fn, ios::in | ios::binary);
    struct stat results;
    if (stat(fn, &results) == 0){
        data = new char[results.st_size];
        file.read((char *) data,results.st_size );
        file.close();
        return (T *) data;
    }
    return NULL;
}

template <class T> void writeFile(const char *fn, T *data, size_t n_el){
    ofstream myFile (fn, ios::out | ios::binary);
    myFile.write ((char *) data, n_el*sizeof(T));
    myFile.close();
}