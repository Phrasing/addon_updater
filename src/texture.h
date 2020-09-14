#ifndef ADDON_UPDATER_TEXTURE_H
#define ADDON_UPDATER_TEXTURE_H
#pragma once

namespace addon_updater {


auto UploadTexture(uint8_t* texture_data, int32_t width,
                                          int32_t height, int32_t channels) -> ImTextureID;

auto LoadTexture(uint8_t* input_data, size_t input_size, int32_t* output_w,
                     int32_t* output_h, int32_t* output_channels) -> uint8_t*;

auto ResizeTexture(uint8_t* texture_data, int32_t actual_w,
                       int32_t acutual_h, int32_t resize_w, int32_t resize_h,
                       int32_t channels) -> uint8_t*;

}  // namespace addon_updater_texture

#endif  // !ADDON_UPDATER_TEXTURE_H
