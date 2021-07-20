#ifndef fft_hpp
#define fft_hpp

#include "data_types/Area.hpp"

struct FFTResult {
    int wave_length;
    float auto_correlation_peak;
    float frequency;
    const char* note_name;
    char note_octave;
    char note_cents;
};

void fft_init(int window_length);
void fft_compute(Area in, Area out, FFTResult* result);
void fft_destroy();

#endif
