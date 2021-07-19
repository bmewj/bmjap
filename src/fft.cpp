#include "fft.hpp"
#include <fftw3/fftw3.h>
#include <cmath>
#include <string.h>

static int window_length;
static int N;
static fftw_complex* buf_in;
static fftw_complex* buf_out;
static fftw_plan plan_1;
static fftw_plan plan_2;

void fft_init(int window_length) {
    window_length = window_length;
    N = (int)exp2(ceil(log2(window_length)));

    buf_in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
    buf_out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
    plan_1 = fftw_plan_dft_1d(N, buf_in, buf_out, FFTW_FORWARD, FFTW_MEASURE);
    plan_2 = fftw_plan_dft_1d(N, buf_out, buf_in, FFTW_BACKWARD, FFTW_MEASURE);
}

void fft_compute(Area in, Area out, char* message) {

    // ... fill in with data
    {
        auto ptr     = buf_in;
        auto ptr_end = buf_in + N;
        // Copy the window into our double array
        while (in < in.end) {
            (*ptr)[0] = *in++;
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
        auto ptr     = buf_out;
        auto ptr_end = buf_out + N;
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
        auto ptr_in = buf_in;
        auto ptr_out = out;
        auto scalar = 1.0 / buf_in[0][0]; // the first sample is the max
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
        for (auto max = *ptr; ptr < ptr.end; ++ptr) {
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
        auto note_value_cents = (note_value - note_value_fixed) * 100;
        auto note_value_octave = note_value_fixed / 12;
        auto note = NOTE_NAMES[note_value_fixed % 12];
        sprintf(message, "%fHz = %s%d + %f", frequency, note, note_value_octave, note_value_cents);
    }
}

void fft_destroy() {
    fftw_destroy_plan(plan_1);
    fftw_destroy_plan(plan_2);
    fftw_free(buf_in);
    fftw_free(buf_out);
}
