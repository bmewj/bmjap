#ifndef window_reader_hpp
#define window_reader_hpp

#include "data_types/Area.hpp"
#include "data_types/ring_buffer.hpp"

typedef void (*window_callback_t)(Area window, long start_count);
void window_reader_init(RingBufferState* ring_buffer, double window_time, int sample_rate, window_callback_t callback);
void window_reader_start();
void window_reader_stop();
void window_reader_destroy();

#endif
