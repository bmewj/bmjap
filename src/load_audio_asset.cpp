#include "load_audio_asset.hpp"
#include <ddui/util/get_asset_filename>
#include "stb_vorbis.c"

inline float convert_sample(short sample) {
    return (float)((double)sample / 32768.0);
}

void convert_samples(int num_samples, short* input, float* output) {
    short* in_ptr     = input;
    short* in_ptr_end = input + num_samples;
    float* out_ptr  = output;
    while (in_ptr < in_ptr_end) {
        *out_ptr++ = convert_sample(*in_ptr++);
    }
}

AudioAsset load_audio_asset(const char* asset_name) {
    // Load our audio file
    auto file_name = get_asset_filename(asset_name);
    int num_channels, sample_rate;
    short* input;
    auto num_samples = stb_vorbis_decode_filename(file_name.c_str(), &num_channels, &sample_rate, &input);

    if (num_samples <= 0) {
        printf("Failed to load our rhodes.ogg file\n");
        exit(1);
    }
    
    float* output = new float[num_samples * num_channels];
    convert_samples(num_samples * num_channels, input, output);
    
    AudioAsset result;
    result.left  = Area(output,     num_samples, 2);
    result.right = Area(output + 1, num_samples, 2);
    return result;
}
