#ifndef envelope_detect_hpp
#define envelope_detect_hpp

#include "data_types/Area.hpp"

struct EnvelopeDetectState {
    float attack_threshold;
    float attack_lookahead;
    float release_threshold;
    float release_hold_time;

    bool envelope_active;
    long time_attack;
    long time_release;
};

void envelope_detect_init(EnvelopeDetectState* state);
void envelope_detect_compute(EnvelopeDetectState* state, int sample_rate, long start_time, Area lvl_in, float pitch_detect_confidence);

#endif
