/*
 * Single translation unit for stb_image and stb_image_write.
 *
 * Download the headers from https://github.com/nothings/stb:
 *   stb_image.h        -> this directory
 *   stb_image_write.h  -> this directory
 *
 * This file compiles them exactly once. All other .cpp files
 * include the headers *without* the IMPLEMENTATION define.
 */

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_PSD
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_HDR

#ifdef __has_include
#if __has_include("stb_image.h")
#include "stb_image.h"
#define GOPOST_HAS_STB_IMAGE 1
#endif
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef __has_include
#if __has_include("stb_image_write.h")
#include "stb_image_write.h"
#define GOPOST_HAS_STB_IMAGE_WRITE 1
#endif
#endif
