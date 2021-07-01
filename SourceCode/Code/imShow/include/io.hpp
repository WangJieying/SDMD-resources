#ifndef IO_GUARD_HPP
#define IO_GUARD_HPP

#include <iostream>
#include <fstream>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
using namespace std;

template <class T> int readFile(const char *fn, T **out)
{
    char* data;
    ifstream file(fn, ios::in | ios::binary);
    struct stat results;
    if (stat(fn, &results) == 0)
    {
        data = (char *) malloc(results.st_size + 1);
        file.read((char *) data, results.st_size);
        file.close();
        data[results.st_size] = '\0';
        *out = (T*) data;
        return results.st_size / sizeof (T);
    }

    return 0;
}

template <class T> void writeFile(const char *fn, T *data, size_t n_el)
{
    ofstream myFile (fn, ios::out | ios::binary);
    myFile.write ((char *) data, n_el * sizeof(T));
    myFile.close();
}
#endif
