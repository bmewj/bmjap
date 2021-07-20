list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_client.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/load_audio_asset.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/load_audio_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/peak_image.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/peak_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fft.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fft.cpp
)
add_subdirectory(data_types)
set(SOURCES ${SOURCES} PARENT_SCOPE)
