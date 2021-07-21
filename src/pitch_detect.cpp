#include "pitch_detect.hpp"
#include <cmath>

void pitch_detect_init_state(PitchDetectState* state, double window_time, int sample_rate) {

    state->window_time = window_time;
    state->sample_rate = sample_rate;
    state->window_length = (int)(window_time * sample_rate);

    state->fft_state = fft_init(state->window_length);

    state->data = new float[state->window_length];
}

void pitch_detect_compute(PitchDetectState* state, Area window_in, Area* window_out, PitchDetectResult* result) {
    *window_out = Area(state->data, window_in.num_samples(), 1);
    fft_compute(state->fft_state, window_in, *window_out);

    auto ptr = *window_out;
    
    // Skip first peak until we cross below zero
    for (; ptr < ptr.end && *ptr > 0.0; ++ptr) {}

    // Find max value
    int max_i = 0;
    auto max = *ptr;
    for (; ptr < ptr.end; ++ptr) {
        if (max < *ptr) {
            max = *ptr;
            max_i = ptr.ptr - window_out->ptr;
        }
    }

    if (max_i == 0) {
        result->wave_length = 0;
        result->confidence = 0;
        result->frequency = 0.0;
        result->note_name = "";
        result->note_octave = 0;
        result->note_cents = 0;
        return;
    }

    auto frequency = state->sample_rate / max_i;

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

void pitch_detect_destroy(PitchDetectState* state) {
    fft_destroy(state->fft_state);
    delete[] state->data;
}
