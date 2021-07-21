#include <ddui/core>
#include <ddui/app>
#include <fftw3/fftw3.h>
#include <cmath>
#include <atomic>

#include "audio_client.hpp"
#include "pitch_detect.hpp"
#include "peak_image.hpp"
#include "data_types/ring_buffer.hpp"

static Image img_1;
static Image img_2;
static Image img_3;

static PitchDetectState pd_state;

constexpr double WINDOW_TIME = 0.025;
constexpr int SAMPLE_RATE = 44100;
constexpr int WINDOW_LENGTH = WINDOW_TIME * SAMPLE_RATE;

static RingBufferState ring_buffer;
static RingBufferReaderState ring_buffer_reader;

void update() {
    
    auto ANIMATION_ID = (void*)0xF0;
    if (!ddui::animation::is_animating(ANIMATION_ID)) {
        ddui::animation::start(ANIMATION_ID);
    }
    
    Area slice_in, slice_out;
    PitchDetectResult result;
    int count = pitch_detect_read(&pd_state, &slice_in, &slice_out, &result);
    if (slice_in.ptr == NULL) {
        return;
    }

    static bool img_generated = false;
    if (!img_generated) {
        img_generated = true;

        img_1 = create_image(ddui::view.width, 200);
        img_2 = create_image(ddui::view.width, 200);
        img_3 = create_image(ddui::view.width, 100);
    }

    // Draw incoming waveform
    {
        create_peak_image(img_1, slice_in, ddui::rgb(0x009900));

        ddui::save();
        ddui::translate(0, 0);

        auto paint = ddui::image_pattern(0, 0, img_1.width, img_1.height, 0.0f, img_1.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_1.width, img_1.height);
        ddui::fill_paint(paint);
        ddui::fill();
        ddui::restore();
    }
    
    // Draw auto-correlation waveform
    {
        create_peak_image(img_2, slice_out, ddui::rgb(0xff0000));
        
        ddui::save();
        ddui::translate(0, 200);

        auto paint = ddui::image_pattern(0, 0, img_2.width, img_2.height, 0.0f, img_2.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_2.width, img_2.height);
        ddui::fill_paint(paint);
        ddui::fill();
        
        if (result.confidence > 0.5) {
            ddui::begin_path();
            ddui::stroke_color(ddui::rgb(0x0000ff));
            ddui::stroke_width(1.0);
            auto x = (result.wave_length / (float)WINDOW_LENGTH) * img_2.width;
            auto y = (1.0 - result.confidence) * 0.5 * img_2.height;
            ddui::move_to(0, y);
            ddui::line_to(x, y);
            ddui::stroke();
        }
            
        ddui::restore();
    }
    
    // Draw full ring buffer contents
    {
        auto area = Area(ring_buffer.buffer, ring_buffer.buffer_size, ring_buffer.step);
        create_peak_image(img_3, area, ddui::Color(ddui::rgb(0x888888)));
        
        ddui::save();
        ddui::translate(0, 400);

        auto paint = ddui::image_pattern(0, 0, img_3.width, img_3.height, 0.0f, img_3.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_3.width, img_3.height);
        ddui::fill_paint(paint);
        ddui::fill();
        
        ddui::restore();
    }
    
    // Draw text overlay
    {
        ddui::save();
        ddui::translate(0, 500);
        
        static char message_1[32];
        static char message_2[32];
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

        ddui::font_face("mono");
        ddui::font_size(26.0);
        ddui::fill_color(ddui::rgb(0x000000));
        float asc, desc, line_h;
        ddui::text_metrics(&asc, &desc, &line_h);
        ddui::text(10, line_h, message_1, NULL);
        ddui::text(10, 2 * line_h, message_2, NULL);
        
        ddui::restore();
    }
}

void read_callback(int num_samples, int num_areas, Area* areas) {

    // ... write input into RingBuffer for software monitoring
    {
        auto ptr_in  = areas[0];
        auto ptr_out = ring_buffer_start_write(&ring_buffer, num_samples);
        while (ptr_in < ptr_in.end) {
            *ptr_out++ = *ptr_in++;
        }
        ring_buffer_end_write(&ring_buffer, num_samples);
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

    if (ring_buffer_can_read(&ring_buffer, &ring_buffer_reader, num_samples)) {
        auto ptr_in = ring_buffer_read(&ring_buffer, &ring_buffer_reader, num_samples);
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
    
    ring_buffer_init(&ring_buffer, SAMPLE_RATE * 4.0);
    ring_buffer_reader_init(&ring_buffer, &ring_buffer_reader);

    pitch_detect_init_state(&pd_state, &ring_buffer, WINDOW_TIME, SAMPLE_RATE);
    pitch_detect_start(&pd_state);
    init_audio_client(SAMPLE_RATE, read_callback, write_callback);

    ddui::app_run();

    pitch_detect_destroy(&pd_state);
    ring_buffer_destroy(&ring_buffer);

    return 0;
}
