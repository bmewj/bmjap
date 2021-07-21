#ifndef ring_buffer_hpp
#define ring_buffer_hpp

#include <atomic>
#include "Area.hpp"

struct RingBufferState {
    float* buffer;
    int step;
    int buffer_size;
    std::atomic_long write_point;
};

typedef long RingBufferReaderState;

void ring_buffer_init(RingBufferState* rb, int buffer_size, int num_areas = 1);
void ring_buffer_start_write(RingBufferState* rb, int num_samples, int num_areas, Area* areas);
Area ring_buffer_start_write(RingBufferState* rb, int num_samples);
void ring_buffer_end_write(RingBufferState* rb, int num_samples);
void ring_buffer_reader_init(RingBufferState* rb, RingBufferReaderState* rbr);
bool ring_buffer_can_read(RingBufferState* rb, RingBufferReaderState* rbr, int num_samples);
void ring_buffer_read(RingBufferState* rb, RingBufferReaderState* rbr, int num_samples, int num_areas, Area* areas);
Area ring_buffer_read(RingBufferState* rb, RingBufferReaderState* rbr, int num_samples);
void ring_buffer_destroy(RingBufferState* rb);

#endif
