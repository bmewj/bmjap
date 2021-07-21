#include "audio_client.hpp"

#include <portaudio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

static ReadCallback  app_read_callback;
static WriteCallback app_write_callback;

static PaStream *pa_stream;

constexpr int BUFFER_SIZE = 32;
constexpr int SAMPLE_RATE = 44100;
constexpr int NUM_IN_CHANNELS = 1;
constexpr int NUM_OUT_CHANNELS = 2;

static int patestCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData) {

    /* Cast data passed through stream to our structure. */
    auto in = (float*)inputBuffer;
    auto out = (float*)outputBuffer;

    Area in_areas[NUM_IN_CHANNELS];
    for (int i = 0; i < NUM_IN_CHANNELS; ++i) {
        in_areas[i] = Area(in + i, framesPerBuffer, NUM_IN_CHANNELS);
    }

    Area out_areas[NUM_OUT_CHANNELS];
    for (int i = 0; i < NUM_OUT_CHANNELS; ++i) {
        out_areas[i] = Area(out + i, framesPerBuffer, NUM_OUT_CHANNELS);
    }

    app_read_callback (framesPerBuffer, NUM_IN_CHANNELS, in_areas);
    app_write_callback(framesPerBuffer, NUM_OUT_CHANNELS, out_areas);

    return 0;
}

int init_audio_client(int sample_rate, ReadCallback read_callback_, WriteCallback write_callback_) {
    app_read_callback = std::move(read_callback_);
    app_write_callback = std::move(write_callback_);

    char *stream_name = NULL;
    double latency = (double)BUFFER_SIZE / sample_rate;

    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream(
        &pa_stream,
        NUM_IN_CHANNELS,          /* no input channels */
        NUM_OUT_CHANNELS,          /* no output channels */
        paFloat32,  /* 32 bit floating point output */
        SAMPLE_RATE,
        BUFFER_SIZE,
        patestCallback, /* this is your callback function */
        NULL); /* This is a pointer that will be passed to your callback*/
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    /* Start the stream */
    err = Pa_StartStream(pa_stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    return 0;
}

void destroy_audio_client() {
    PaError err;

    err = Pa_StopStream(pa_stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
    }
    Pa_Terminate();
}
