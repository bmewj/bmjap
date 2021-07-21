#include "window_reader.hpp"
#include <atomic>
#include <pthread.h>

static double window_time;
static int sample_rate;
static int window_length;

static RingBufferState* ring_buffer;
static RingBufferReaderState ring_buffer_reader;
static window_callback_t callback;

static float* window_data;
static pthread_t thread;
static std::atomic_bool running;

static void* window_reader_thread(void* ptr);

void window_reader_init(RingBufferState* ring_buffer_, double window_time_, int sample_rate_, window_callback_t callback_) {

    window_time = window_time_;
    sample_rate = sample_rate_;
    window_length = (int)(window_time * sample_rate);

    ring_buffer = ring_buffer_;
    ring_buffer_reader_init(ring_buffer, &ring_buffer_reader);

    callback = callback_;

    window_data = new float[window_length * ring_buffer->step];
}

void window_reader_start() {
    pthread_create(&thread, NULL, window_reader_thread, NULL);
}

void window_reader_stop() {
    if (running) {
        running = false;
        pthread_join(thread, NULL);
    }
}

void window_reader_destroy() {
    delete[] window_data;
}

void* window_reader_thread(void* ptr) {
    running = true;

    while (running.load()) {

        // Read in a new window
        auto window = Area(window_data, window_length, ring_buffer->step);
        auto ptr_out = window;
        while (ptr_out < ptr_out.end) {
            auto ptr_in = ring_buffer_read(ring_buffer, &ring_buffer_reader, ptr_out.num_samples());
            while (ptr_in < ptr_in.end) {
                *ptr_out++ = *ptr_in++;
            }
        }

        callback(window, ring_buffer_reader - window.num_samples());
    }

    return NULL;
}
