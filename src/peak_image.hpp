#ifndef peak_image_hpp
#define peak_image_hpp

#include "data_types/Area.hpp"
#include <ddui/core>

struct Image {
    int image_id;
    int width;
    int height;
    unsigned char* data;
};

Image create_image(int width, int height);

void create_peak_image(Image img, Area area, ddui::Color color);

#endif
