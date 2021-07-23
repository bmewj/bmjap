#include <ddui/core>
#include <ddui/app>
#include <fftw3/fftw3.h>
#include <cmath>
#include <atomic>
#include <mutex>

#include "audio_client.hpp"
#include "pitch_detect.hpp"
#include "peak_image.hpp"
#include "window_reader.hpp"
#include "levels.hpp"
#include "envelope_detect.hpp"
#include "data_types/ring_buffer.hpp"

static PitchDetectState pd_state;

constexpr double WINDOW_TIME = 0.025;
constexpr int SAMPLE_RATE = 44100;
constexpr int WINDOW_LENGTH = WINDOW_TIME * SAMPLE_RATE;

static RingBufferState ring_buffer;
static RingBufferReaderState ring_buffer_reader;

static float* sample_buffer;
static int sample_buffer_size;
static Area sample_buffer_area;
static RingBufferReaderState sample_reader;

static std::atomic_int sample_playback_count;

static std::mutex mutex;
static int count;
static PitchDetectResult result;
static Image img_window;
static Image img_ac;
static Image img_ring;

static LevelsState lvl_state;
static EnvelopeDetectState env_state;

void update() {
    auto ANIMATION_ID = (void*)0xF0;
    if (!ddui::animation::is_animating(ANIMATION_ID)) {
        ddui::animation::start(ANIMATION_ID);
    }
    
    if (ddui::key_state.mods & ddui::keyboard::MOD_SHIFT) {
        if (sample_playback_count == -1) {
            sample_playback_count = 0;
        }
    } else {
        sample_playback_count = -1;
    }

    PitchDetectResult result_copy;
    {
        std::lock_guard<std::mutex> lg(mutex);
        result_copy = result;

        static long previous_count = 0;
        if (previous_count != count) {
            previous_count = count;
            ddui::update_image(img_window.image_id, img_window.data);
            ddui::update_image(img_ac.image_id, img_ac.data);
            ddui::update_image(img_ring.image_id, img_ring.data);
        }
    }

    // Draw incoming waveform
    {
        ddui::save();
        ddui::translate(0, 0);

        auto paint = ddui::image_pattern(0, 0, img_window.width, img_window.height, 0.0f, img_window.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_window.width, img_window.height);
        ddui::fill_paint(paint);
        ddui::fill();
        ddui::restore();
    }

    // Draw auto-correlation waveform
    {
        ddui::save();
        ddui::translate(0, 200);

        auto paint = ddui::image_pattern(0, 0, img_ac.width, img_ac.height, 0.0f, img_ac.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_ac.width, img_ac.height);
        ddui::fill_paint(paint);
        ddui::fill();
        
        if (result.confidence > 0.5) {
            ddui::begin_path();
            ddui::stroke_color(ddui::rgb(0x0000ff));
            ddui::stroke_width(1.0);
            auto x = (result.wave_length / (float)WINDOW_LENGTH) * img_ac.width;
            auto y = (1.0 - result.confidence) * 0.5 * img_ac.height;
            ddui::move_to(0, y);
            ddui::line_to(x, y);
            ddui::stroke();
        }
            
        ddui::restore();
    }
    
    // Draw full ring buffer contents
    {
        ddui::save();
        ddui::translate(0, 400);

        auto paint = ddui::image_pattern(0, 0, img_ring.width, img_ring.height, 0.0f, img_ring.image_id, 1.0f);
        ddui::begin_path();
        ddui::rect(0, 0, img_ring.width, img_ring.height);
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
        if (result_copy.confidence > 0.5) {
            sprintf(
                message_1,
                "%fHz = %s%d + %d",
                result_copy.frequency,
                result_copy.note_name,
                (int)result_copy.note_octave,
                (int)result_copy.note_cents
            );
            sprintf(
                message_2,
                "Confidence: %f",
                result_copy.confidence
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

void window_callback(Area area, long count_) {
    std::lock_guard<std::mutex> lg(mutex);

    Area ac_area, lvl_area;
    levels_compute(&lvl_state, area, &lvl_area);
    pitch_detect_compute(&pd_state, area, &ac_area, &result);

    auto envelope_active_pre = env_state.envelope_active;
    envelope_detect_compute(&env_state, SAMPLE_RATE, count_, lvl_area, result.confidence);

    if (!envelope_active_pre && env_state.envelope_active) {
        // Start sampling
        sample_buffer_area = Area(sample_buffer, sample_buffer_size, 1);

        // Clear buffer
        {
            auto ptr = sample_buffer_area;
            while (ptr < ptr.end) {
                *ptr++ = 0.0;
            }
        }

        if (env_state.time_attack < count_) {
            // Copy in samples from before the window started (handling lookahead)
            RingBufferReaderState rbr;
            rbr = env_state.time_attack;
            auto in = ring_buffer_read(&ring_buffer, &rbr, (int)(count_ - env_state.time_attack));
            while (in < in.end) {
                *sample_buffer_area++ = *in++;
            }
        }
    } else if (envelope_active_pre && !env_state.envelope_active) {
        // Stop sampling
    }

    if (env_state.envelope_active) {
        // Continue sampling
        auto from = (int)(env_state.time_attack - count_);
        if (from < 0) {
            from = 0;
        }
        auto ptr = area;
        while (ptr < ptr.end) {
            *sample_buffer_area++ = *ptr++;
        }
    }

    render_peak_image(img_window, area, ddui::rgb(0x009900));
    render_peak_image(img_ac, ac_area, ddui::rgb(0xff0000));

    auto vis_area = Area(sample_buffer, SAMPLE_RATE * 10, 1);
    render_peak_image(img_ring, vis_area, ddui::Color(ddui::rgb(0x888888)));

    count = count_;
}

void read_callback(int num_samples, int num_areas, Area* areas) {
    auto ptr_in  = areas[0];
    auto ptr_out = ring_buffer_start_write(&ring_buffer, num_samples);
    while (ptr_in < ptr_in.end) {
        *ptr_out++ = *ptr_in++;
    }
    ring_buffer_end_write(&ring_buffer, num_samples);
}

void write_callback(int num_samples, int num_areas, Area* areas) {

    if (!ring_buffer_can_read(&ring_buffer, &ring_buffer_reader, num_samples)) {
        return;
    }
    auto ptr_in = ring_buffer_read(&ring_buffer, &ring_buffer_reader, num_samples);
    
    auto count = sample_playback_count.load();
    if (count != -1) {
        auto area = Area(&sample_buffer[0] + count, sample_buffer_size - count, 1);
        auto ptr_out_l = areas[0];
        auto ptr_out_r = areas[1];
        while (area < area.end && ptr_out_l < ptr_out_l.end) {
            *ptr_out_l++ = *area;
            *ptr_out_r++ = *area++;
        }
        if (area == area.end) {
            sample_playback_count = -1;
        } else if (sample_playback_count.load() != -1) {
            sample_playback_count += area.ptr - (&sample_buffer[0] + count);
        }
        return;
    }
    
    if (ring_buffer_can_read(&ring_buffer, &ring_buffer_reader, num_samples)) {
        auto ptr_out_l = areas[0];
        auto ptr_out_r = areas[1];
        while (ptr_in < ptr_in.end) {
            *ptr_out_l++ = *ptr_in;
            *ptr_out_r++ = *ptr_in++;
        }
    }

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

    img_window = create_image(700, 200);
    img_ac = create_image(700, 200);
    img_ring = create_image(700, 100);

    levels_init(&lvl_state, SAMPLE_RATE, 0.1, WINDOW_LENGTH); // 0.1s = 100ms decay time

    ring_buffer_init(&ring_buffer, SAMPLE_RATE * 4.0);
    ring_buffer_reader_init(&ring_buffer, &ring_buffer_reader);

    window_reader_init(&ring_buffer, WINDOW_TIME, SAMPLE_RATE, window_callback);
    window_reader_start();

    pitch_detect_init_state(&pd_state, WINDOW_TIME, SAMPLE_RATE);
    envelope_detect_init(&env_state);

    sample_buffer = new float[SAMPLE_RATE * 30];
    sample_buffer_size = SAMPLE_RATE * 30;
    sample_buffer_area = Area(sample_buffer, sample_buffer_size, 1);

    init_audio_client(SAMPLE_RATE, read_callback, write_callback);

    ddui::app_run();

    pitch_detect_destroy(&pd_state);
    levels_destroy(&lvl_state);
    ring_buffer_destroy(&ring_buffer);

    return 0;
}
