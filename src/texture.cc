// clang-format off
#include "pch.h"
#include "texture.h"
// clang-format on

namespace addon_updater {
ImTextureID UploadTexture(uint8_t* texture_data, int width, int height,
                          int channels) {
  if (!texture_data) {
    fprintf(stderr, "Error: failed to upload texture %p\n", &texture_data);
    return nullptr;
  }

  defer { DestroyTexture(texture_data); };

  uint32_t texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  int internal_format = 0;
  switch (channels) {
    case 3:
      internal_format = GL_RGB;
      break;
    case 4:
      internal_format = GL_RGBA;
      break;
    default: {
      glDeleteTextures(1, &texture);
      fprintf(stderr, "Error: failed to upload texture %p\n", &texture_data);
      return nullptr;
    }
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0,
               internal_format, GL_UNSIGNED_BYTE, texture_data);

  return reinterpret_cast<void*>(static_cast<uintptr_t>(texture));
}

uint8_t* LoadTexture(uint8_t* image_data, size_t image_size, int* out_width,
                     int* out_height, int* output_channels) {
  if (!image_data) {
    fprintf(stderr, "Error: failed to load texture %p\n", &image_data);
    return nullptr;
  }

  uint8_t* result = nullptr;
  if (!(result = stbi_load_from_memory(image_data, image_size, out_width,
                                       out_height, output_channels, 0))) {
    fprintf(stderr, "Error: failed to load texture %p\n", &image_data);
    return nullptr;
  }

  return result;
}

uint8_t* ResizeTexture(uint8_t* texture_data, int width, int height,
                       int resized_width, int resized_height, int channels) {
  if (!texture_data) return nullptr;

  defer { DestroyTexture(texture_data); };

  auto* output_pixels = reinterpret_cast<uint8_t*>(
      malloc(resized_width * resized_height * channels * sizeof(uint8_t)));

  if (!stbir_resize_uint8(texture_data, width, height, 0, output_pixels,
                          resized_width, resized_height, 0, channels)) {
    fprintf(stderr, "Error: failed to resize texture %p\n", &texture_data);
    return nullptr;
  }

  return output_pixels;
}

void DestroyTexture(uint8_t* texture) {
  stbi_image_free(texture);
  texture = nullptr;
}

}  // namespace addon_updater_texture