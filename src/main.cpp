#include <ddui/core>
#include <ddui/app>
#include <ddui/util/get_asset_filename>

#include "stb_vorbis.c"
#include "audio_client.hpp"

static Area file_left_channel;
static Area file_right_channel;

void update() {

    // Writing hello world
    ddui::font_face("regular");
    ddui::font_size(48.0);
    ddui::fill_color(ddui::rgb(0x000000));
    ddui::text_align(ddui::align::CENTER | ddui::align::MIDDLE);
    ddui::text(ddui::view.width / 2, ddui::view.height / 2, "Hello, world!", NULL);

}

void read_callback(int num_samples, int num_areas, Area* areas) {

}

void write_callback(int num_samples, int num_areas, Area* areas) {

    for (int n = 0; n < num_areas; ++n) {
        auto area = areas[n];
        while (area < area.end) {
            *area++ = 0.0;
        }
    }
    
    auto& in_l = file_left_channel;
    auto& in_r = file_right_channel;
    auto out_l = areas[0];
    auto out_r = areas[1];
    while (in_l < in_l.end && out_l < out_l.end) {
        *out_l++ = *in_l++;
        *out_r++ = *in_r++;
    }

}

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

int main(int argc, const char** argv) {

    // ddui (graphics and UI system)
    if (!ddui::app_init(700, 300, "BMJ's Audio Programming", update)) {
        printf("Failed to init ddui.\n");
        return 1;
    }

    // Type faces
    ddui::create_font("regular", "SFRegular.ttf");
    ddui::create_font("medium", "SFMedium.ttf");
    ddui::create_font("bold", "SFBold.ttf");
    ddui::create_font("thin", "SFThin.ttf");
    ddui::create_font("mono", "PTMono.ttf");

    // Load our audio file
    auto file_name = get_asset_filename("rhodes.ogg");
    int num_channels, sample_rate;
    short* input;
    auto num_samples = stb_vorbis_decode_filename(file_name.c_str(), &num_channels, &sample_rate, &input);

    if (num_samples <= 0) {
        printf("Failed to load our rhodes.ogg file\n");
        return 1;
    }
    
    float* output = new float[num_samples * num_channels];
    convert_samples(num_samples * num_channels, input, output);
    
    file_left_channel  = Area(output,     num_samples, 2);
    file_right_channel = Area(output + 1, num_samples, 2);

    init_audio_client(sample_rate, read_callback, write_callback);

    ddui::app_run();

    return 0;
}
