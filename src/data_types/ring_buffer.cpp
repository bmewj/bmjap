#include "ring_buffer.hpp"
#include <cmath>
#include <assert.h>
#include <time.h>

void ring_buffer_init(RingBufferState* rb, int buffer_size, int num_areas) {
    buffer_size = (int)exp2(ceil(log2(buffer_size)));
    rb->buffer = new float[buffer_size * num_areas];
    rb->step = num_areas;
    rb->buffer_size = buffer_size;
    rb->write_point = 0;
}

void ring_buffer_start_write(RingBufferState* rb, int num_samples, int num_areas, Area* areas) {
    assert(num_samples <= rb->buffer_size);
    assert(num_areas <= rb->step);

    auto write_point_mod = rb->write_point.load() % rb->buffer_size;
    assert((rb->buffer_size - write_point_mod) % num_samples == 0);
    if (write_point_mod == 0) {
        for (int i = 0; i < num_areas; ++i) {
            areas[i] = Area(&rb->buffer[i], num_samples, rb->step);
        }
    } else {
        for (int i = 0; i < num_areas; ++i) {
            areas[i] = Area(&rb->buffer[i] + write_point_mod * rb->step, num_samples, rb->step);
        }
    }
}

Area ring_buffer_start_write(RingBufferState* rb, int num_samples) {
    Area area;
    ring_buffer_start_write(rb, num_samples, 1, &area);
    return area;
}

void ring_buffer_end_write(RingBufferState* rb, int num_samples) {
    rb->write_point += num_samples;
}

void ring_buffer_reader_init(RingBufferState* rb, RingBufferReaderState* rbr) {
    *rbr = rb->write_point.load();
}

bool ring_buffer_can_read(RingBufferState* rb, RingBufferReaderState* rbr, int num_samples) {
    assert(num_samples <= rb->buffer_size);

    auto write_point = rb->write_point.load();
    return (*rbr + num_samples <= write_point);
}

void ring_buffer_read(RingBufferState* rb, RingBufferReaderState* rbr, int num_samples, int num_areas, Area* areas) {
    assert(num_samples <= rb->buffer_size);
    assert(num_areas <= rb->step);

    constexpr double SAMPLE_RATE_GUESS = 44100.0;

    // Sleep until enough samples are available
    long write_point;
    while (true) {
        write_point = rb->write_point.load();
        auto readible = write_point - *rbr;
        if (readible > 0) {
            break;
        }

        auto remaining_in_seconds = (num_samples - readible) / SAMPLE_RATE_GUESS;
        timespec ts;
        ts.tv_sec = (int)remaining_in_seconds;
        ts.tv_nsec = (long)((remaining_in_seconds - ts.tv_sec) * 1000000000.0);
        nanosleep(&ts, NULL);
    }

    int read_point_mod = *rbr % rb->buffer_size;
    if (read_point_mod + num_samples <= rb->buffer_size) {
        for (int i = 0; i < num_areas; ++i) {
            areas[i] = Area(&rb->buffer[i] + read_point_mod * rb->step, num_samples, rb->step);
        }
    } else if (read_point_mod == rb->buffer_size) {
        for (int i = 0; i < num_areas; ++i) {
            areas[i] = Area(&rb->buffer[i], num_samples, rb->step);
        }
    } else {
        int available = rb->buffer_size - read_point_mod;
        for (int i = 0; i < num_areas; ++i) {
            areas[i] = Area(&rb->buffer[i], available, rb->step);
        }
    }

    *rbr += num_samples;
}

Area ring_buffer_read(RingBufferState* rb, RingBufferReaderState* rbr, int num_samples) {
    Area area;
    ring_buffer_read(rb, rbr, num_samples, 1, &area);
    return area;
}

void ring_buffer_destroy(RingBufferState* rb) {
    delete[] rb->buffer;
}
