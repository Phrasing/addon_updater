#ifndef ADDON_UPDATER_TEXTURE_H
#define ADDON_UPDATER_TEXTURE_H
#pragma once

namespace addon_updater {

ImTextureID UploadTexture(uint8_t* texture_data, int width, int height,
                          int channels);

uint8_t* LoadTexture(uint8_t* image_data, size_t image_size, int* out_width,
                     int* out_height, int* output_channels);

uint8_t* ResizeTexture(uint8_t* texture_data, int width, int height,
                       int resized_width, int resized_height, int channels);

void DestroyTexture(uint8_t* texture);

}  // namespace addon_updater_texture

#endif  // !ADDON_UPDATER_TEXTURE_H
