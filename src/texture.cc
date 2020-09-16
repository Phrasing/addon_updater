// clang-format off
#include "pch.h"
#include "texture.h"
// clang-format on

namespace addon_updater {

ImTextureID UploadTexture(uint8_t* texture_data, int32_t width, int32_t height,
                          int32_t channels) {
  GLuint texture;
  glGenTextures(1, &texture);

  if (!texture_data) return nullptr;

  glBindTexture(GL_TEXTURE_2D, texture);

  GLint internal_format;
  switch (channels) {
    case 3:
      internal_format = GL_RGB;
      break;
    case 4:
      internal_format = GL_RGBA;
      break;
    default:
      glDeleteTextures(1, &texture);
      return nullptr;
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0,
               internal_format, GL_UNSIGNED_BYTE, texture_data);

  DestroyTexture(texture_data);

  return reinterpret_cast<void*>(static_cast<uintptr_t>(texture));
}

uint8_t* LoadTexture(uint8_t* input_data, size_t input_size, int32_t* output_w,
                     int32_t* output_h, int32_t* output_channels) {
  if (!input_data) return nullptr;
  return stbi_load_from_memory(input_data, input_size, output_w, output_h,
                               output_channels, 0);
}

uint8_t* ResizeTexture(uint8_t* texture_data, int32_t actual_w,
                       int32_t acutual_h, int32_t resize_w, int32_t resize_h,
                       int32_t channels) {
  if (!texture_data) return nullptr;

  auto* output_pixels = reinterpret_cast<uint8_t*>(
      STBI_MALLOC(static_cast<size_t>(resize_w * resize_h * channels)));

  stbir_resize_uint8(texture_data, actual_w, acutual_h, 0, output_pixels,
                     resize_w, resize_h, 0, channels);

  DestroyTexture(texture_data);

  return output_pixels;
}

void DestroyTexture(uint8_t* texture) {
  stbi_image_free(texture);
  texture = nullptr;
}

}  // namespace addon_updater_texture