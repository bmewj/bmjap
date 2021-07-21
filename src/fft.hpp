#ifndef fft_hpp
#define fft_hpp

#include "data_types/Area.hpp"

struct FFTState;

FFTState* fft_init(int window_length);
void fft_compute(FFTState* state, Area in, Area out);
void fft_destroy(FFTState* state);

#endif
