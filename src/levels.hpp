#ifndef levels_hpp
#define levels_hpp

#include "data_types/Area.hpp"

constexpr float DECAY_TIME = 0.1; // 100 milliseconds

struct LevelsState {
    float level;
    float decay_per_frame;

    float* data;
    Area data_area;
};

void levels_init(LevelsState* state, int sample_rate, float decay_time, int window_size);
void levels_compute(LevelsState* state, Area in, Area* out);
void levels_destroy(LevelsState* state);

#endif
