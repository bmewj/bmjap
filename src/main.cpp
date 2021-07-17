#include <ddui/core>
#include <ddui/app>
#include <ddui/util/get_asset_filename>
#include <fftw3/fftw3.h>
#include <cmath>

#include "stb_vorbis.c"
#include "audio_client.hpp"

static Area file_left_channel;
//static Area file_right_channel;

static Area playback_1;
static Area playback_2;


struct Image {
    int image_id;
    int width;
    int height;
    unsigned char* data;
};

Image create_image(int width, int height) {
    Image img;
    img.width = width;
    img.height = height;
    img.data = new unsigned char[4 * width * height];
    memset(img.data, 0xff, 4 * width * height);
    img.image_id = ddui::create_image_from_rgba(width, height, 0, img.data);
    return img;
}

static Image img_1;
static Image img_2;
static Image img_3;
static Image img_4;
static Image img_5;
static Area slice;
static Area offset_slice;
static Area sum_slice;
static Area another_slice;
static int offset = 0;
static void create_peak_image(Image img, Area area, ddui::Color color);

constexpr double WINDOW_TIME = 0.025;
constexpr int SAMPLE_RATE = 44100;
constexpr int WINDOW_LENGTH = WINDOW_TIME * SAMPLE_RATE;

void update() {

    static bool img_generated = false;
    if (!img_generated) {
        img_generated = true;

        slice = file_left_channel + 1000;
        slice.end = slice.ptr + WINDOW_LENGTH * slice.step;
        img_1 = create_image(ddui::view.width, 100);
        img_2 = create_image(ddui::view.width, 100);
        img_3 = create_image(ddui::view.width, 200);
        img_4 = create_image(ddui::view.width, 200);
        img_5 = create_image(ddui::view.width, 200);
        create_peak_image(img_1, slice, ddui::rgb(0xff0000));
        create_peak_image(img_2, slice, ddui::rgb(0x0000ff));
        
        offset_slice = Area(new float[WINDOW_LENGTH], WINDOW_LENGTH, 1);
        sum_slice = Area(new float[WINDOW_LENGTH], WINDOW_LENGTH, 1);
    }

    // Draw first (fixed) waveform
    {
        auto paint = ddui::image_pattern(0, 0, img_1.width, img_1.height, 0.0f, img_1.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_1.width, img_1.height);
        ddui::fill_paint(paint);
        ddui::fill();
    }
    
    // Draw second (movable) waveform
    {
        static bool is_dragging = false;
        static int start_offset;
        ddui::save();
        ddui::translate(0, 100);
        
        if (!ddui::mouse_state.accepted && is_dragging) {
            is_dragging = false;
            playback_1 = file_left_channel;
            playback_2 = file_left_channel + (offset >= 0 ? offset : -offset);
        } else if (is_dragging) {
            ddui::set_cursor(ddui::CURSOR_CLOSED_HAND);
            offset = start_offset + (ddui::mouse_state.x - ddui::mouse_state.initial_x) * (WINDOW_LENGTH / (double)ddui::view.width);
        } else if (ddui::mouse_hit(0, 0, img_2.width, img_2.height)) {
            is_dragging = true;
            start_offset = offset;
            ddui::mouse_hit_accept();
            ddui::set_cursor(ddui::CURSOR_CLOSED_HAND);
        } else if (ddui::mouse_over(0, 0, img_2.width, img_2.height)) {
            ddui::set_cursor(ddui::CURSOR_OPEN_HAND);
        }
        
        auto x = offset * (ddui::view.width / (double)WINDOW_LENGTH);
        ddui::translate(x, 0);

        auto paint = ddui::image_pattern(0, 0, img_2.width, img_2.height, 0.0f, img_2.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_1.width, img_1.height);
        ddui::fill_paint(paint);
        ddui::fill();
        ddui::restore();
    }

    // Draw offset slice
    {
        static bool did_init = false;
        static int previous_offset = 0;
        if (!did_init) {
            Area ptr = sum_slice;
            while (ptr < ptr.end) {
                *ptr++ = 0.0;
            }
        }
        if (!did_init || previous_offset != offset) {

            // Compute auto correlation at offset "offset"
            {
                Area ptr_in_1 = slice;
                Area ptr_in_2 = slice;
                Area ptr_out = offset_slice;
                if (offset >= 0) {
                    ptr_in_1 += offset;
                    ptr_out += offset;
                } else {
                    ptr_in_2 += -offset;
                }

                while (ptr_in_1 < ptr_in_1.end && ptr_out < ptr_out.end && ptr_in_2 < ptr_in_2.end) {
                    *ptr_out++ = *ptr_in_1++ * *ptr_in_2++;
                }
            }

            // Sum it all up
            {
                Area ptr_offset = offset_slice + (offset > 0 ? offset : 0);
                double sum = 0.0;
                static double sum_all_squared = 0.0;
                for (; ptr_offset < ptr_offset.end; ++ptr_offset) {
                    sum += *ptr_offset;
                }

                if (offset == 0) {
                    sum_all_squared = sum;
                }
                
                auto offset_min = offset < previous_offset ? offset : previous_offset;
                auto offset_max = offset > previous_offset ? offset : previous_offset;
                Area ptr_out = sum_slice + offset_min;
                Area ptr_out_end = sum_slice + offset_max;
                while (ptr_out < ptr_out.end && ptr_out < ptr_out_end.ptr) {
                    *ptr_out++ = sum / sum_all_squared;
                }
            }
            
            previous_offset = offset;
            create_peak_image(img_3, sum_slice, ddui::rgb(0x009900));
        }
        did_init = true;

        ddui::save();
        ddui::translate(0, 200);

        auto paint = ddui::image_pattern(0, 0, img_3.width, img_3.height, 0.0f, img_3.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_3.width, img_3.height);
        ddui::fill_paint(paint);
        ddui::fill();
        ddui::restore();
    }
    
    // Draw another slice
    {
        static bool did_init = false;
        if (!did_init) {
            did_init = true;
            create_peak_image(img_5, another_slice, ddui::rgb(0xff0000));
        }

        ddui::save();
        ddui::translate(0, 400);

        auto paint = ddui::image_pattern(0, 0, img_5.width, img_5.height, 0.0f, img_5.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_5.width, img_5.height);
        ddui::fill_paint(paint);
        ddui::fill();
        ddui::restore();
    }
}

void read_callback(int num_samples, int num_areas, Area* areas) {

}

void write_callback(int num_samples, int num_areas, Area* areas) {

//    constexpr int SAMPLE_RATE = 44100;
//    double frequency = (double)SAMPLE_RATE / max_i;
//
//    double time_period = SAMPLE_RATE / frequency;
//
//    auto out_l = areas[0];
//    auto out_r = areas[1];
//    static int i = 0;
//    while (out_l < out_l.end) {
//        *out_l++ = std::sin(2.0 * 3.14159 * i++ / time_period);
//    }
    
    for (int n = 0; n < num_areas; ++n) {
        auto area = areas[n];
        while (area < area.end) {
            *area++ = 0.0;
        }
    }

    auto& in_1 = playback_1;
    auto& in_2 = playback_2;
    auto out_l = areas[0];
    auto out_r = areas[1];
    while (in_1 < in_1.end && in_2 < in_2.end && out_l < out_l.end) {
        auto sample = 0.5 * *in_1++ + 0.5 * *in_2++;
        *out_l++ = sample;
        *out_r++ = sample;
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

static void do_fft();

int main(int argc, const char** argv) {

    // ddui (graphics and UI system)
    if (!ddui::app_init(700, 600, "BMJ's Audio Programming", update)) {
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
    auto file_name = get_asset_filename("C3.ogg");
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
//    file_right_channel = Area(output + 1, num_samples, 2);

    playback_1 = file_left_channel;
    playback_2 = file_left_channel;
    
    do_fft();

    init_audio_client(sample_rate, read_callback, write_callback);

    ddui::app_run();

    return 0;
}

static void color_to_bytes(ddui::Color in, unsigned char* out);

void create_peak_image(Image img, Area area, ddui::Color color) {

    unsigned char fg_bytes[4];
    unsigned char bg_bytes[4];
    color_to_bytes(color, fg_bytes);
    color_to_bytes(color, bg_bytes);
    bg_bytes[3] = 0x00; // transparent

    auto width = img.width;
    auto height = img.height;
    auto data = img.data;
    
    // Fill with background
    for (long i = 0; i < 4 * width * height; i += 4) {
        data[i  ] = bg_bytes[0];
        data[i+1] = bg_bytes[1];
        data[i+2] = bg_bytes[2];
        data[i+3] = bg_bytes[3];
    }
    
    int num_samples = ((area.end - area.ptr) / area.step);

    // Fill foreground
    auto ptr = area;
    float last_sample = *ptr;
    for (auto x = 0; x < width; ++x) {

        // Get the value
        int y0, y1;
        {
            int i_end = ((double)(x + 1) / (double)width) * num_samples;
            auto ptr_end = area.ptr + i_end * area.step;
            if (ptr_end > ptr.end) {
                ptr_end = ptr.end;
            }
            float min_value = last_sample;
            float max_value = last_sample;
            for (; ptr < ptr_end; ++ptr) {
                last_sample = *ptr;
                if (min_value > last_sample) {
                    min_value = last_sample;
                }
                if (max_value < last_sample) {
                    max_value = last_sample;
                }
            }
            y0 = height * (1.0 - (max_value + 1.0) * 0.5);
            y1 = height * (1.0 - (min_value + 1.0) * 0.5);
        }

        if (y0 < 0) {
            y0 = 0;
        }
        if (y0 >= height) {
            y0 = height;
        }
        if (y1 < 0) {
            y1 = 0;
        }
        if (y1 >= height) {
            y1 = height;
        }

        // Fill in the pixels
        for (auto y = y0; y <= y1; ++y) {
            auto i = 4 * (width * y + x);
            data[i  ] = fg_bytes[0];
            data[i+1] = fg_bytes[1];
            data[i+2] = fg_bytes[2];
            data[i+3] = fg_bytes[3];
        }
    }

    ddui::update_image(img.image_id, data);
}

static void color_to_bytes(ddui::Color in, unsigned char* out) {
    out[0] = (unsigned char)(in.r * 255.0f);
    out[1] = (unsigned char)(in.g * 255.0f);
    out[2] = (unsigned char)(in.b * 255.0f);
    out[3] = (unsigned char)(in.a * 255.0f);
}

void do_fft() {
    auto N = (int)exp2(ceil(log2(WINDOW_LENGTH)));

    fftw_plan plan, plan_2;
    auto in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
    auto out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
    plan = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    plan_2 = fftw_plan_dft_1d(N, out, in, FFTW_BACKWARD, FFTW_ESTIMATE);

    // .. fill in with data
    {
        auto window = file_left_channel + 1000;
        window.end = window.ptr + WINDOW_LENGTH * window.step;
        auto ptr_in = in;
        auto ptr_in_end = in + N;
        // Copy the window into our double array
        while (window < window.end) {
            (*ptr_in)[0] = *window++;
            (*ptr_in)[1] = 0.0;
            ptr_in++;
        }
        // Zero pad the input
        while (ptr_in < ptr_in_end) {
            (*ptr_in)[0] = 0.0;
            (*ptr_in)[1] = 0.0;
            ptr_in++;
        }
    }

    printf("transforming...\n");
    fftw_execute(plan);
    {
        auto ptr = out;
        auto ptr_end = out + N;
        auto scalar = 1.0 / std::sqrt((double)N);
        for (; ptr < ptr_end; ++ptr) {
            auto real = (*ptr)[0] * scalar;
            auto imag = (*ptr)[1] * scalar;
            (*ptr)[0] = real * real + imag * imag;
            (*ptr)[1] = 0.0;
        }
        ptr = out;
    }

    printf("inverse transforming...\n");
    fftw_execute(plan_2);
    {
        auto ptr = (double*)in;
        auto ptr_end = (double*)in + N*2;
        auto scalar = 1.0 / *ptr; // the first sample is the max
        for (; ptr < ptr_end; ++ptr) {
            *ptr *= scalar;
        }
    }
    printf("done.\n");

    // Saving output to area
    another_slice = Area(new float[WINDOW_LENGTH], WINDOW_LENGTH, 1);
    {
        auto slice_out = another_slice;
//        another_slice.end = another_slice.ptr + 200;
        auto ptr_in = in;
        while (slice_out < slice_out.end) {
            *slice_out++ = (*ptr_in++)[0];
        }
    }

    // Computing frequency using auto correlation
    {
        auto ptr = in;
        auto ptr_end = in + WINDOW_LENGTH;
        
        // Skip first peak until we cross below zero
        for (; ptr < ptr_end && (*ptr)[0] > 0.0; ++ptr) {}

        // Find max value
        int max_i = 0;
        for (auto max = (*ptr)[0]; ptr < ptr_end; ++ptr) {
            if (max < (*ptr)[0]) {
                max = (*ptr)[0];
                max_i = ptr - in;
            }
        }

        constexpr double SAMPLE_RATE = 44100.0;
        auto frequency = SAMPLE_RATE / max_i;

        constexpr const char* NOTE_NAMES[12] = {
            "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"
        };

        // A4 = 440Hz -> A0 = 27.5Hz
        constexpr double FREQ_A0 = 27.5;
        auto note_value = 12.0 * log(frequency / FREQ_A0) / log(2.0);
        auto note_value_fixed = (int)round(note_value);
        auto note_value_cents = (note_value - note_value_fixed) * 100;
        auto note_value_octave = note_value_fixed / 12;
        auto note = NOTE_NAMES[note_value_fixed % 12];
        printf("Detected: %s%d + %f (%fHz)\n", note, note_value_octave, note_value_cents, frequency);
    }

    // Computing frequency using FFT frequency bins
//    {
//        auto ptr = out;
//        auto ptr_end = out + N;
//
//        // Find max value
//        int max_i = 0;
//        for (auto max = (*ptr)[0]; ptr < ptr_end; ++ptr) {
//            if (max < (*ptr)[0]) {
//                max = (*ptr)[0];
//                max_i = ptr - out;
//            }
//        }
//
//        constexpr double SAMPLE_RATE = 44100.0;
//        auto frequency = max_i * (SAMPLE_RATE / N);
//        printf("%fHz\n", frequency);
//    }

    // ... repeat as needed
    fftw_destroy_plan(plan);
    fftw_destroy_plan(plan_2);
    fftw_free(in);
    fftw_free(out);
}
