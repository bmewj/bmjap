#ifndef AudioAsset_hpp
#define AudioAsset_hpp

#include "data_types/Area.hpp"

struct AudioAsset {
    Area left, right;
};

AudioAsset load_audio_asset(const char* asset_name);

#endif
