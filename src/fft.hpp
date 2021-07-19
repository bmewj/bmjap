#ifndef fft_hpp
#define fft_hpp

#include "data_types/Area.hpp"

void fft_init(int window_length);
void fft_compute(Area in, Area out, char* message);
void fft_destroy();

#endif
