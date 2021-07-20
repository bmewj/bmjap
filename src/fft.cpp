#include "fft.hpp"
#include <fftw3/fftw3.h>
#include <cmath>
#include <string.h>

static int window_length;
static int N;
static fftw_complex* buf_A;
static fftw_complex* buf_B;
static fftw_plan plan_1;
static fftw_plan plan_2;

void fft_init(int window_length) {
    window_length = window_length;
    N = (int)exp2(ceil(log2(window_length)));

    buf_A = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
    buf_B = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
    plan_1 = fftw_plan_dft_1d(N, buf_A, buf_B, FFTW_FORWARD,  FFTW_MEASURE);
    plan_2 = fftw_plan_dft_1d(N, buf_B, buf_A, FFTW_BACKWARD, FFTW_MEASURE);
}

void fft_compute(Area in, Area out, FFTResult* result) {

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
        auto ptr     = buf_A;
        auto ptr_end = buf_A + N;
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
    fftw_execute(plan_1);

    // ... square the output
    {
        auto ptr     = buf_B;
        auto ptr_end = buf_B + N;
        auto scalar = 1.0 / std::sqrt((double)N);
        for (; ptr < ptr_end; ++ptr) {
            auto real = (*ptr)[0] * scalar;
            auto imag = (*ptr)[1] * scalar;
            (*ptr)[0] = real * real + imag * imag;
            (*ptr)[1] = 0.0;
        }
    }

    // ... execute reverse FFT
    fftw_execute(plan_2);

    // ... write output
    {
        auto ptr_in = buf_A;
        auto ptr_out = out;
        auto scalar = 1.0 / buf_A[0][0]; // the first sample is the max
        while (ptr_out < ptr_out.end) {
            *ptr_out++ = (*ptr_in++)[0] * scalar;
        }
    }

    // ... compute frequency using auto-correlation
    {
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


        constexpr double SAMPLE_RATE = 44100.0;
        auto frequency = SAMPLE_RATE / max_i;

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
        result->auto_correlation_peak = max;
        result->frequency = frequency;
        result->note_name = note;
        result->note_octave = (char)note_value_octave;
        result->note_cents = (char)note_value_cents;
    }
}

void fft_destroy() {
    fftw_destroy_plan(plan_1);
    fftw_destroy_plan(plan_2);
    fftw_free(buf_A);
    fftw_free(buf_B);
}
