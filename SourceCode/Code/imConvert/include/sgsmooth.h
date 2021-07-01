#ifndef _SGSMOOTH_PLUGIN_H
#define _SGSMOOTH_PLUGIN_H
#include <stdlib.h>     /* exit, EXIT_FAILURE */

#ifdef __cplusplus
extern "C" 
{
#endif

extern double *calc_sgsmooth(const int ndat, double *input,
                             const int window, const int order);

extern double *calc_sgsderiv(const int ndat, double *input,
                             const int window, const int order, 
                             const double delta);

    

#ifdef __cplusplus
}
#endif

#endif