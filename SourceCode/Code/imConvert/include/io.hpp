#ifndef IO_GUARD
#define IO_GUARD

#include <stdio.h>
//char *readFile(const char *file);
template <class T> T * readFile(const char *fn);
template <class T> void writeFile(const char *fn, T *data, size_t n_el);
#endif
