#include "envelope_detect.hpp"

void envelope_detect_init(EnvelopeDetectState* state) {
    state->attack_threshold = 0.1;
    state->attack_lookahead = 0.05;
    state->release_threshold = 0.01;
    state->release_hold_time = 0.05;

    state->envelope_active = false;
    state->time_attack = -1;
    state->time_release = -1;
}

void envelope_detect_compute(EnvelopeDetectState* state, int sample_rate, long start_time, Area lvl_in, float pitch_detect_confidence) {

    auto attack_threshold = state->attack_threshold;
    auto attack_lookahead = (int)(state->attack_lookahead * sample_rate);
    auto release_threshold = state->release_threshold;
    auto release_hold_time = (int)(state->release_hold_time * sample_rate);

    auto ptr = lvl_in;
    while (ptr < ptr.end) {

        if (!state->envelope_active) {
            for (; ptr < ptr.end; ++ptr) {
                if (*ptr >= attack_threshold) {
                    break;
                }
            }
            if (ptr < ptr.end) {
                state->envelope_active = true;
                state->time_attack = start_time + (ptr.ptr - lvl_in.ptr) - attack_lookahead;
                state->time_release = -1;
                continue;
            }
        } else {
            if (pitch_detect_confidence > 0.5) {
                break;
            }
            for (; ptr < ptr.end; ++ptr) {
                if (*ptr < release_threshold) {
                    break;
                }
            }
            if (ptr < ptr.end) {
                state->envelope_active = false;
                state->time_release = start_time + (ptr.ptr - lvl_in.ptr) + release_hold_time;
            }
        }

    }

}
