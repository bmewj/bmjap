#include "pitch_detect.hpp"
#include <unistd.h>
#include <cmath>

static void* pitch_detect_thread(void* ptr);
static void sleep(double seconds);
static void compute_result(double sample_rate, Area out, PitchDetectResult* result);

void pitch_detect_init_state(PitchDetectState* state, RingBuffer* ring_buffer, double window_time, int sample_rate) {

    state->window_time = window_time;
    state->sample_rate = sample_rate;
    state->window_length = (int)(window_time * sample_rate);

    RingBufferReader::init(&state->ring_buffer_reader, ring_buffer);
    state->fft_state = fft_init(state->window_length);

    // Allocate all the slices
    state->data = new float[state->window_length * NUM_BUFFERS * 2];
    auto ptr = state->data;
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        state->windows_in[i] = Area(ptr, state->window_length, 1);
        ptr += state->window_length;
    }
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        state->windows_out[i] = Area(ptr, state->window_length, 1);
        ptr += state->window_length;
    }

    // Prepare slice counters
    state->window_count = 0;
}

void pitch_detect_start(PitchDetectState* state) {
    pthread_create(&state->thread, NULL, pitch_detect_thread, (void*)state);
}

void pitch_detect_stop(PitchDetectState* state) {
    if (state->running) {
        state->running = false;
        pthread_join(state->thread, NULL);
    }
}

void pitch_detect_destroy(PitchDetectState* state) {
    fft_destroy(state->fft_state);
    delete[] state->data;
}

int pitch_detect_read(PitchDetectState* state, Area* in, Area* out, PitchDetectResult* result) {
    auto window_count = state->window_count.load() - 1;
    if (window_count < 0) {
        *in = Area();
        *out = Area();
        *result = PitchDetectResult();
    } else {
        *in  = state->windows_in [window_count % NUM_BUFFERS];
        *out = state->windows_out[window_count % NUM_BUFFERS];
        *result = state->results[window_count % NUM_BUFFERS];
    }
    return window_count;
}

void* pitch_detect_thread(void* ptr) {
    auto state = (PitchDetectState*)ptr;
    state->running = true;

    while (state->running.load()) {
        auto window_count = state->window_count.load();

        // Read in a new window
        auto ptr_out = state->windows_in[window_count % NUM_BUFFERS];
        while (ptr_out < ptr_out.end) {
            auto ptr_in = state->ring_buffer_reader.read(ptr_out.num_samples());
            while (ptr_in < ptr_in.end) {
                *ptr_out++ = *ptr_in++;
            }
        }

        // Compute output window
        auto slice_in = state->windows_in[window_count % NUM_BUFFERS];
        auto slice_out = state->windows_out[(window_count + 1) % NUM_BUFFERS];
        auto result = &state->results[(window_count + 1) % NUM_BUFFERS];
        fft_compute(state->fft_state, slice_in, slice_out);
        compute_result(state->sample_rate, slice_out, result);

        state->window_count++;
    }

    return NULL;
}

void sleep(double seconds) {
    timespec ts;
    ts.tv_sec = (int)seconds;
    ts.tv_nsec = (long)((seconds - ts.tv_sec) * 1000000000.0);
    nanosleep(&ts, NULL);
}

void compute_result(double sample_rate, Area out, PitchDetectResult* result) {
    auto ptr = out;
    
    // Skip first peak until we cross below zero
    for (; ptr < ptr.end && *ptr > 0.0; ++ptr) {}

    // Find max value
    int max_i = 0;
    auto max = *ptr;
    for (; ptr < ptr.end; ++ptr) {
        if (max < *ptr) {
            max = *ptr;
            max_i = ptr.ptr - out.ptr;
        }
    }

    auto frequency = sample_rate / max_i;

    constexpr const char* NOTE_NAMES[12] = {
        "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"
    };

    // A4 = 440Hz -> A0 = 27.5Hz
    constexpr double FREQ_A0 = 27.5;
    auto note_value = 12.0 * log(frequency / FREQ_A0) / log(2.0);
    auto note_value_fixed = (int)round(note_value);
    auto note_value_cents = (int)round((note_value - note_value_fixed) * 100);
    auto note_value_octave = note_value_fixed / 12;
    auto note = NOTE_NAMES[note_value_fixed % 12];

    // Fill result
    result->wave_length = max_i;
    result->confidence = max;
    result->frequency = frequency;
    result->note_name = note;
    result->note_octave = (char)note_value_octave;
    result->note_cents = (char)note_value_cents;
}
