#ifndef StereoBuffer_hpp
#define StereoBuffer_hpp

#include <memory>

struct StereoBuffer {
    int sample_rate;
    int number_of_samples;
    std::unique_ptr<float[]> samples;
};

#endif
