#include "levels.hpp"
#include <assert.h>

void levels_init(LevelsState* state, int sample_rate, float decay_time, int window_size) {
    state->level = 0.0;
    state->decay_per_frame = 1.0 / (sample_rate * decay_time);

    state->data = new float[window_size];
    state->data_area = Area(state->data, window_size, 1);
}

void levels_compute(LevelsState* state, Area in, Area* out_) {
    assert(state->data_area.num_samples() >= in.num_samples());

    auto& level = state->level;
    auto  decay_per_frame = state->decay_per_frame;
    auto  out = state->data_area;

    *out_ = Area(state->data, in.num_samples(), 1);

    while (in < in.end) {
        level -= decay_per_frame;
        if (level < *in) {
            level = *in;
        }
        if (level < 0.0) {
            level = 0.0;
        }
        *out++ = level;
        in++;
    }
}

void levels_destroy(LevelsState* state) {
    delete[] state->data;
}
