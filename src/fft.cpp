#include "fft.hpp"
#include <fftw3/fftw3.h>
#include <cmath>
#include <string.h>

struct FFTState {
    int window_length;
    int N;
    fftw_complex* buf_A;
    fftw_complex* buf_B;
    fftw_plan plan_1;
    fftw_plan plan_2;
};

FFTState* fft_init(int window_length) {
    auto state = new FFTState;

    state->window_length = window_length;
    state->N = (int)exp2(ceil(log2(window_length)));

    state->buf_A = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * state->N);
    state->buf_B = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * state->N);
    state->plan_1 = fftw_plan_dft_1d(state->N, state->buf_A, state->buf_B, FFTW_FORWARD,  FFTW_MEASURE);
    state->plan_2 = fftw_plan_dft_1d(state->N, state->buf_B, state->buf_A, FFTW_BACKWARD, FFTW_MEASURE);
    
    return state;
}

void fft_compute(FFTState* state, Area in, Area out) {

    // ... determine input scalar
    double scalar = 1.0;
    {
        auto ptr = in;
        auto max = *ptr;
        for (; ptr < ptr.end; ++ptr) {
            if (max < *ptr) {
                max = *ptr;
            }
        }
        if (max != 0.0) {
            scalar = 1.0 / (double)max;
        }
    }

    // ... fill in with data
    {
        auto ptr     = state->buf_A;
        auto ptr_end = state->buf_A + state->N;
        // Copy the window into our double array
        while (in < in.end) {
            (*ptr)[0] = *in++ * scalar;
            (*ptr)[1] = 0.0;
            ptr++;
        }
        // Zero pad the input
        while (ptr < ptr_end) {
            (*ptr)[0] = 0.0;
            (*ptr)[1] = 0.0;
            ptr++;
        }
    }

    // ... execute forward FFT
    fftw_execute(state->plan_1);

    // ... square the output
    {
        auto ptr     = state->buf_B;
        auto ptr_end = state->buf_B + state->N;
        auto scalar = 1.0 / std::sqrt((double)state->N);
        for (; ptr < ptr_end; ++ptr) {
            auto real = (*ptr)[0] * scalar;
            auto imag = (*ptr)[1] * scalar;
            (*ptr)[0] = real * real + imag * imag;
            (*ptr)[1] = 0.0;
        }
    }

    // ... execute reverse FFT
    fftw_execute(state->plan_2);

    // ... write output
    {
        auto ptr_in = state->buf_A;
        auto ptr_out = out;
        auto scalar = 1.0 / state->buf_A[0][0]; // the first sample is the max
        while (ptr_out < ptr_out.end) {
            *ptr_out++ = (*ptr_in++)[0] * scalar;
        }
    }
}

void fft_destroy(FFTState* state) {
    fftw_destroy_plan(state->plan_1);
    fftw_destroy_plan(state->plan_2);
    fftw_free(state->buf_A);
    fftw_free(state->buf_B);
    delete state;
}
