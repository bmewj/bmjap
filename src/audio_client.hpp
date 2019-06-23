#ifndef audio_client_hpp
#define audio_client_hpp

#include "data_types/Area.hpp"
#include <functional>

typedef std::function<void(int num_samples, int num_areas, Area*)> WriteCallback;
typedef std::function<void(int num_samples, int num_areas, Area*)> ReadCallback;
int init_audio_client(int sample_rate, ReadCallback read_callback, WriteCallback write_callback);
void destroy_audio_client();

#endif
