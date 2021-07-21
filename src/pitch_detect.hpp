#ifndef pitch_detect_hpp
#define pitch_detect_hpp

#include "fft.hpp"
#include <atomic>
#include <pthread.h>
#include "data_types/RingBuffer.hpp"

struct PitchDetectResult {
    int wave_length;
    float confidence;
    float frequency;
    const char* note_name;
    char note_octave;
    char note_cents;
};

constexpr int NUM_BUFFERS = 2;

struct PitchDetectState {
    double window_time;
    int sample_rate;
    int window_length;

    RingBufferReader ring_buffer_reader;
    FFTState* fft_state;

    Area windows_in[NUM_BUFFERS];
    Area windows_out[NUM_BUFFERS];
    PitchDetectResult results[NUM_BUFFERS];
    std::atomic_int window_count;

    float* data;
    pthread_t thread;
    std::atomic_bool running;
};

void pitch_detect_init_state(PitchDetectState* state, RingBuffer* ring_buffer, double window_time, int sample_rate);
void pitch_detect_start(PitchDetectState* state);
void pitch_detect_stop(PitchDetectState* state);
void pitch_detect_destroy(PitchDetectState* state);

int  pitch_detect_read(PitchDetectState* state, Area* in, Area* out, PitchDetectResult* result);

#endif
