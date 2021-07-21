#ifndef RingBuffer_hpp
#define RingBuffer_hpp

#include <memory>
#include <atomic>
#include "Area.hpp"
#include <assert.h>
#include <unistd.h>

struct RingBuffer {
    std::unique_ptr<float[]> buffer;
    int step;
    int buffer_size;
    std::atomic_long write_point;

    // Constructor
    static void init(RingBuffer* rb, int buffer_size, int step = 1);

    // Writing
    void start_write(int num_samples, int num_areas, Area* areas) const;
    Area start_write(int num_samples) const;
    void end_write(int num_samples);
};

inline void RingBuffer::init(RingBuffer* rb, int buffer_size_, int step_) {
    rb->buffer = std::unique_ptr<float[]>(new float[buffer_size_ * step_]);
    rb->step = step_;
    rb->buffer_size = buffer_size_;
    rb->write_point = 0;
}

inline void RingBuffer::start_write(int num_samples, int num_areas, Area* areas) const {
    assert(num_samples <= buffer_size);
    assert(num_areas <= step);

    auto write_point_mod = write_point.load() % buffer_size;
    assert((buffer_size - write_point_mod) % num_samples == 0);
    if (write_point_mod == 0) {
        for (int i = 0; i < num_areas; ++i) {
            areas[i] = Area(&buffer[i], num_samples, step);
        }
    } else {
        for (int i = 0; i < num_areas; ++i) {
            areas[i] = Area(&buffer[i] + write_point_mod * step, num_samples, step);
        }
    }
}

inline Area RingBuffer::start_write(int num_samples) const {
    Area area;
    start_write(num_samples, 1, &area);
    return area;
}

inline void RingBuffer::end_write(int num_samples) {
    write_point += num_samples;
}

struct RingBufferReader {
    RingBuffer* ring_buffer;
    long read_point;

    // Constructor
    static void init(RingBufferReader* ring_buffer_reader, RingBuffer* ring_buffer);

    // Reading
    bool can_read(int num_samples) const;
    void read(int num_samples, int num_areas, Area* areas);
    Area read(int num_samples);
};

inline void RingBufferReader::init(RingBufferReader* rbr, RingBuffer* ring_buffer) {
    rbr->ring_buffer = ring_buffer;
    rbr->read_point = ring_buffer->write_point.load();
}

inline bool RingBufferReader::can_read(int num_samples) const {
    assert(num_samples <= ring_buffer->buffer_size);

    auto write_point = ring_buffer->write_point.load();
    return (read_point + num_samples <= write_point);
}

inline void RingBufferReader::read(int num_samples, int num_areas, Area* areas) {
    assert(num_samples <= ring_buffer->buffer_size);
    assert(num_areas <= ring_buffer->step);

    constexpr double SAMPLE_RATE_GUESS = 44100.0;

    // Sleep until enough samples are available
    long write_point;
    while (true) {
        write_point = ring_buffer->write_point.load();
        auto readible = write_point - read_point;
        if (readible > 0) {
            break;
        }

        auto remaining_in_seconds = (num_samples - readible) / SAMPLE_RATE_GUESS;
        timespec ts;
        ts.tv_sec = (int)remaining_in_seconds;
        ts.tv_nsec = (long)((remaining_in_seconds - ts.tv_sec) * 1000000000.0);
        nanosleep(&ts, NULL);
    }

    int read_point_mod = read_point % ring_buffer->buffer_size;
    if (read_point_mod + num_samples <= ring_buffer->buffer_size) {
        for (int i = 0; i < num_areas; ++i) {
            areas[i] = Area(&ring_buffer->buffer[i] + read_point_mod * ring_buffer->step, num_samples, ring_buffer->step);
        }
    } else if (read_point_mod == ring_buffer->buffer_size) {
        for (int i = 0; i < num_areas; ++i) {
            areas[i] = Area(&ring_buffer->buffer[i], num_samples, ring_buffer->step);
        }
    } else {
        int available = ring_buffer->buffer_size - read_point_mod;
        for (int i = 0; i < num_areas; ++i) {
            areas[i] = Area(&ring_buffer->buffer[i], available, ring_buffer->step);
        }
    }

    read_point += num_samples;
}

inline Area RingBufferReader::read(int num_samples) {
    Area area;
    read(num_samples, 1, &area);
    return area;
}

#endif
