#include <ddui/core>
#include <ddui/app>
#include <fftw3/fftw3.h>
#include <cmath>
#include <atomic>

#include "audio_client.hpp"
#include "fft.hpp"
#include "peak_image.hpp"

static Image img_1;
static Image img_2;
static Image img_3;

static Area slice_in_0;
static Area slice_in_1;
static Area slice_write_ptr;
static std::atomic_int slice_count(0);

static Area ac_slice;

constexpr double WINDOW_TIME = 0.025;
constexpr int SAMPLE_RATE = 44100;
constexpr int WINDOW_LENGTH = WINDOW_TIME * SAMPLE_RATE;

void update() {
    
    auto ANIMATION_ID = (void*)0xF0;
    if (!ddui::animation::is_animating(ANIMATION_ID)) {
        ddui::animation::start(ANIMATION_ID);
    }

    static bool img_generated = false;
    if (!img_generated) {
        img_generated = true;

        img_1 = create_image(ddui::view.width, 200);
        img_2 = create_image(ddui::view.width, 200);
        img_3 = create_image(ddui::view.width, 200);
        
        ac_slice = Area(new float[WINDOW_LENGTH], WINDOW_LENGTH, 1);
    }

    // Draw offset slice
    {
        static bool did_init = false;
        static int previous_count = 0;
        auto count = slice_count.load();
        if (!did_init || previous_count != count) {
            previous_count = count;
            did_init = true;
            create_peak_image(img_1, (count & 1) == 0 ? slice_in_1 : slice_in_0, ddui::rgb(0x009900));
        }

        ddui::save();
        ddui::translate(0, 0);

        auto paint = ddui::image_pattern(0, 0, img_1.width, img_1.height, 0.0f, img_1.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_1.width, img_1.height);
        ddui::fill_paint(paint);
        ddui::fill();
        ddui::restore();
    }
    
    // Draw another slice
    {
        static bool did_init = false;
        static int previous_count = 0;
        static FFTResult result;
        static char message[32];
        auto count = slice_count.load();
        if (!did_init || previous_count != count) {
            previous_count = count;
            did_init = true;
            fft_compute((count & 1) == 0 ? slice_in_1 : slice_in_0, ac_slice, &result);
            sprintf(
                message,
                "%fHz = %s%d + %d",
                result.frequency,
                result.note_name,
                (int)result.note_octave,
                (int)result.note_cents
            );
            create_peak_image(img_3, ac_slice, ddui::rgb(0xff0000));
        }

        ddui::save();
        ddui::translate(0, 200);

        auto paint = ddui::image_pattern(0, 0, img_3.width, img_3.height, 0.0f, img_3.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_3.width, img_3.height);
        ddui::fill_paint(paint);
        ddui::fill();
        
        ddui::begin_path();
        ddui::stroke_color(ddui::rgb(0x0000ff));
        ddui::stroke_width(1.0);
        auto x = (result.wave_length / (float)WINDOW_LENGTH) * img_3.width;
        auto y = (- 0.5 * result.auto_correlation_peak + 0.5) * img_3.height;
        ddui::move_to(0, y);
        ddui::line_to(x, y);
        ddui::stroke();

        ddui::restore();

        ddui::font_face("mono");
        ddui::font_size(26.0);
        ddui::fill_color(ddui::rgb(0x000000));
        ddui::text(10, 450, message, NULL);
    }
}

void read_callback(int num_samples, int num_areas, Area* areas) {
    
    auto& ptr_in  = areas[0];
    auto& ptr_out = slice_write_ptr;
    
    while (ptr_out < ptr_out.end && ptr_in < ptr_in.end) {
        *ptr_out++ = *ptr_in++;
    }

    if (ptr_out >= ptr_out.end) {
        // If we filled in all of the slice we want to swap slices
        // and continue writing to the other slice.
        auto next_slice = (slice_count += 1) & 1;
        ptr_out = next_slice == 0 ? slice_in_0 : slice_in_1;

        while (ptr_out < ptr_out.end && ptr_in < ptr_in.end) {
            *ptr_out++ = *ptr_in++;
        }
    }
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
//
//    auto& in_1 = playback_1;
//    auto& in_2 = playback_2;
//    auto out_l = areas[0];
//    auto out_r = areas[1];
//    while (in_1 < in_1.end && in_2 < in_2.end && out_l < out_l.end) {
//        auto sample = 0.5 * *in_1++ + 0.5 * *in_2++;
//        *out_l++ = sample;
//        *out_r++ = sample;
//    }

}

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
    
    slice_in_0 = Area(new float[WINDOW_LENGTH], WINDOW_LENGTH, 1);
    slice_in_1 = Area(new float[WINDOW_LENGTH], WINDOW_LENGTH, 1);

    fft_init(WINDOW_LENGTH);
    init_audio_client(SAMPLE_RATE, read_callback, write_callback);

    ddui::app_run();

    fft_destroy();

    return 0;
}
