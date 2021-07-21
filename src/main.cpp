#include <ddui/core>
#include <ddui/app>
#include <fftw3/fftw3.h>
#include <cmath>
#include <atomic>

#include "audio_client.hpp"
#include "pitch_detect.hpp"
#include "peak_image.hpp"
#include "data_types/RingBuffer.hpp"

static Image img_1;
static Image img_2;
static Image img_3;

static PitchDetectState pd_state;

constexpr double WINDOW_TIME = 0.025;
constexpr int SAMPLE_RATE = 44100;
constexpr int WINDOW_LENGTH = WINDOW_TIME * SAMPLE_RATE;

static RingBuffer ring_buffer;
static RingBufferReader ring_buffer_reader;

void update() {
    
    auto ANIMATION_ID = (void*)0xF0;
    if (!ddui::animation::is_animating(ANIMATION_ID)) {
        ddui::animation::start(ANIMATION_ID);
    }
    
    Area slice_in, slice_out;
    PitchDetectResult result;
    int count = pitch_detect_read(&pd_state, &slice_in, &slice_out, &result);

    static bool img_generated = false;
    if (!img_generated) {
        img_generated = true;

        img_1 = create_image(ddui::view.width, 200);
        img_2 = create_image(ddui::view.width, 200);
        img_3 = create_image(ddui::view.width, 200);
    }

    // Draw offset slice
    {
        static int previous_count = -1;
        if (previous_count != count && count >= 0) {
            previous_count = count;
            create_peak_image(img_1, slice_in, ddui::rgb(0x009900));
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
        static int previous_count = -1;
        
        static char message_1[32];
        static char message_2[32];
        if (previous_count != count && count >= 0) {
            previous_count = count;
            if (result.confidence > 0.5) {
                sprintf(
                    message_1,
                    "%fHz = %s%d + %d",
                    result.frequency,
                    result.note_name,
                    (int)result.note_octave,
                    (int)result.note_cents
                );
                sprintf(
                    message_2,
                    "Confidence: %f",
                    result.confidence
                );
            } else {
                message_1[0] = '\0';
                message_2[0] = '\0';
            }
            create_peak_image(img_3, slice_out, ddui::rgb(0xff0000));
        }

        ddui::save();
        ddui::translate(0, 200);

        auto paint = ddui::image_pattern(0, 0, img_3.width, img_3.height, 0.0f, img_3.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_3.width, img_3.height);
        ddui::fill_paint(paint);
        ddui::fill();
        
        if (result.confidence > 0.5) {
            ddui::begin_path();
            ddui::stroke_color(ddui::rgb(0x0000ff));
            ddui::stroke_width(1.0);
            auto x = (result.wave_length / (float)WINDOW_LENGTH) * img_3.width;
            auto y = (1.0 - result.confidence) * 0.5 * img_3.height;
            ddui::move_to(0, y);
            ddui::line_to(x, y);
            ddui::stroke();
        }
            
        ddui::restore();

        ddui::font_face("mono");
        ddui::font_size(26.0);
        ddui::fill_color(ddui::rgb(0x000000));
        ddui::text(10, 450, message_1, NULL);
        float asc, desc, line_h;
        ddui::text_metrics(&asc, &desc, &line_h);
        ddui::text(10, 450 + line_h, message_2, NULL);
    }
}

void read_callback(int num_samples, int num_areas, Area* areas) {

    // ... write input into RingBuffer for software monitoring
    {
        auto ptr_in  = areas[0];
        auto ptr_out = ring_buffer.start_write(num_samples);
        while (ptr_in < ptr_in.end) {
            *ptr_out++ = *ptr_in++;
        }
        ring_buffer.end_write(num_samples);
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

    if (ring_buffer_reader.can_read(num_samples)) {
        auto ptr_in = ring_buffer_reader.read(num_samples);
        auto ptr_out_l = areas[0];
        auto ptr_out_r = areas[1];
        while (ptr_in < ptr_in.end) {
            *ptr_out_l++ = *ptr_in;
            *ptr_out_r++ = *ptr_in++;
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
    
    RingBuffer::init(&ring_buffer, ((int)exp2(ceil(log2(WINDOW_LENGTH)))));
    RingBufferReader::init(&ring_buffer_reader, &ring_buffer);

    pitch_detect_init_state(&pd_state, &ring_buffer, WINDOW_TIME, SAMPLE_RATE);
    pitch_detect_start(&pd_state);
    init_audio_client(SAMPLE_RATE, read_callback, write_callback);

    ddui::app_run();

    pitch_detect_destroy(&pd_state);

    return 0;
}
