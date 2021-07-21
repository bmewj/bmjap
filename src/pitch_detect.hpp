#ifndef pitch_detect_hpp
#define pitch_detect_hpp

#include "fft.hpp"

struct PitchDetectResult {
    int wave_length;
    float confidence;
    float frequency;
    const char* note_name;
    char note_octave;
    char note_cents;
};

struct PitchDetectState {
    double window_time;
    int sample_rate;
    int window_length;

    FFTState* fft_state;
    float* data;
};

void pitch_detect_init_state(PitchDetectState* state, double window_time, int sample_rate);
void pitch_detect_compute(PitchDetectState* state, Area window_in, Area* window_out, PitchDetectResult* result);
void pitch_detect_destroy(PitchDetectState* state);

#endif
