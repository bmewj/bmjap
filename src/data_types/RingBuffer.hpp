#ifndef RingBuffer_hpp
#define RingBuffer_hpp

#include <memory>
#include <atomic>
#include "Area.hpp"

struct RingBuffer {
    std::unique_ptr<float[]> buffer;
    int buffer_size;
    std::atomic_int read_point;
    std::atomic_int write_point;

    // Constructor
    RingBuffer(int buffer_size);

    // Reading
    bool can_read(int num_samples) const;
    Area start_read(int num_samples) const;
    void end_read(int num_samples);

    // Writing
    Area start_write(int num_samples) const;
    void end_write(int num_samples);
};

inline RingBuffer::RingBuffer(int buffer_size_) {
    buffer = std::unique_ptr<float[]>(new float[buffer_size_]);
    buffer_size = buffer_size_;
    read_point = 0;
    write_point = 0;
}

inline bool RingBuffer::can_read(int num_samples) const {
    int read_point_copy  = read_point;
    int write_point_copy = write_point;
    if (read_point_copy <= write_point_copy) {
        return (read_point_copy + num_samples <= write_point_copy);
    } else {
        return (num_samples <= write_point_copy);
    }
}

inline Area RingBuffer::start_read(int num_samples) const {
    int read_point_copy = read_point;
    if (read_point_copy + num_samples <= buffer_size) {
        return Area(&buffer[0] + read_point_copy, num_samples, 1);
    } else {
        return Area(&buffer[0], num_samples, 1);
    }
}

inline void RingBuffer::end_read(int num_samples) {
    if (read_point + num_samples <= buffer_size) {
        read_point += num_samples;
    } else {
        read_point = num_samples;
    }
}

inline Area RingBuffer::start_write(int num_samples) const {
    int write_point_copy = write_point;
    if (write_point_copy + num_samples <= buffer_size) {
        return Area(&buffer[0] + write_point_copy, num_samples, 1);
    } else {
        return Area(&buffer[0], num_samples, 1);
    }
}

inline void RingBuffer::end_write(int num_samples) {
    if (write_point + num_samples <= buffer_size) {
        write_point += num_samples;
    } else {
        write_point = num_samples;
    }
}

#endif
