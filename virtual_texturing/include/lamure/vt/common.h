//
// Created by sebastian on 13.11.17.
//

#ifndef VT_COMMON_H
#define VT_COMMON_H

#include <cstdint>
#include <string.h>

namespace vt {

typedef uint64_t id_type;
typedef uint32_t priority_type;

class Config {
public:
  // Sections
  static constexpr const char *TEXTURE_MANAGEMENT = "TEXTURE_MANAGEMENT";
  static constexpr const char *DEBUG = "DEBUG";

  // Texture management fields
  static constexpr const char *TILE_SIZE = "TILE_SIZE";
  static constexpr const char *FILE_PPM = "FILE_PPM";
  static constexpr const char *FILE_MIPMAP = "FILE_MIPMAP";

  static constexpr const char *OPT_RUN_IN_PARALLEL = "OPT_RUN_IN_PARALLEL";
  static constexpr const char *OPT_TILE_ROW_IN_CORE = "OPT_TILE_ROW_IN_CORE";

  static constexpr const char *TEXTURE_FORMAT = "TEXTURE_FORMAT";
  static constexpr const char *TEXTURE_FORMAT_RGBA8 = "RGBA8";
  static constexpr const char *TEXTURE_FORMAT_RGB6 = "RGB6";
  static constexpr const char *TEXTURE_FORMAT_R2 = "R2";

  enum FORMAT_TEXTURE {
    RGBA8, RGB6, R2
  };

  static const FORMAT_TEXTURE which_texture_format(const char *_texture_format) {
      if (strcmp(_texture_format, TEXTURE_FORMAT_RGBA8) != 0)
      {
          return RGBA8;
      }
      else if (strcmp(_texture_format, TEXTURE_FORMAT_RGB6) != 0)
      {
          return RGB6;
      }
      else if (strcmp(_texture_format, TEXTURE_FORMAT_R2) != 0)
      {
          return R2;
      }
      throw std::runtime_error("Unknown texture format");
  }

  // Debug fields
  static constexpr const char *KEEP_INTERMEDIATE_DATA = "KEEP_INTERMEDIATE_DATA";
  static constexpr const char *VERBOSE = "VERBOSE";

  static constexpr const char *UNDEF = "UNDEF";
};

}

#endif //VT_COMMON_H
