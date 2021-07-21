#include "peak_image.hpp"

Image create_image(int width, int height) {
    Image img;
    img.width = width;
    img.height = height;
    img.data = new unsigned char[4 * width * height];
    memset(img.data, 0xff, 4 * width * height);
    img.image_id = ddui::create_image_from_rgba(width, height, 0, img.data);
    return img;
}

static void color_to_bytes(ddui::Color in, unsigned char* out);

void render_peak_image(Image img, Area area, ddui::Color color) {

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
}

static void color_to_bytes(ddui::Color in, unsigned char* out) {
    out[0] = (unsigned char)(in.r * 255.0f);
    out[1] = (unsigned char)(in.g * 255.0f);
    out[2] = (unsigned char)(in.b * 255.0f);
    out[3] = (unsigned char)(in.a * 255.0f);
}

