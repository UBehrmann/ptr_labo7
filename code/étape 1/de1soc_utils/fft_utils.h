#ifndef FFT_UTILS_H
#define FFT_UTILS_H

#include <complex.h>
#include <math.h>

typedef double complex cplx;

void fft(cplx buf[], cplx out[], int n);

#endif // FFT_UTILS_H
