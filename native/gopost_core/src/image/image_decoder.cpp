#include "gopost/image_codec.h"
#include "gopost/engine.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <algorithm>

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#if !defined(__APPLE__)
#ifdef __has_include
#if __has_include("stb_image.h")
#include "stb_image.h"
#define GOPOST_HAS_STB_IMAGE 1
#endif
#endif
#endif

static GopostImageFormat detect_format(const uint8_t* data, size_t len) {
    if (len < 4) return GOPOST_IMAGE_FORMAT_UNKNOWN;

    if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)
        return GOPOST_IMAGE_FORMAT_JPEG;

    if (data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G')
        return GOPOST_IMAGE_FORMAT_PNG;

    if (len >= 12 && data[0] == 'R' && data[1] == 'I' &&
        data[2] == 'F' && data[3] == 'F' &&
        data[8] == 'W' && data[9] == 'E' &&
        data[10] == 'B' && data[11] == 'P')
        return GOPOST_IMAGE_FORMAT_WEBP;

    if (len >= 12) {
        uint32_t box_type = 0;
        memcpy(&box_type, data + 4, 4);
        if (memcmp(data + 4, "ftyp", 4) == 0)
            return GOPOST_IMAGE_FORMAT_HEIC;
    }

    if (data[0] == 'B' && data[1] == 'M')
        return GOPOST_IMAGE_FORMAT_BMP;

    if (len >= 6 && data[0] == 'G' && data[1] == 'I' && data[2] == 'F' &&
        data[3] == '8' && (data[4] == '7' || data[4] == '9') && data[5] == 'a')
        return GOPOST_IMAGE_FORMAT_GIF;

    if (len >= 4 &&
        ((data[0] == 'I' && data[1] == 'I' && data[2] == 0x2A && data[3] == 0x00) ||
         (data[0] == 'M' && data[1] == 'M' && data[2] == 0x00 && data[3] == 0x2A)))
        return GOPOST_IMAGE_FORMAT_TIFF;

    return GOPOST_IMAGE_FORMAT_UNKNOWN;
}

static GopostExifOrientation read_jpeg_orientation(const uint8_t* data, size_t len) {
    if (len < 12 || data[0] != 0xFF || data[1] != 0xD8)
        return GOPOST_ORIENTATION_NORMAL;

    size_t pos = 2;
    while (pos + 4 < len) {
        if (data[pos] != 0xFF) break;
        uint8_t marker = data[pos + 1];

        if (marker == 0xE1) {
            uint16_t seg_len = (uint16_t)(data[pos + 2] << 8) | data[pos + 3];
            if (pos + 4 + seg_len > len) break;

            const uint8_t* exif = data + pos + 4;
            if (seg_len > 8 && memcmp(exif, "Exif\0\0", 6) == 0) {
                const uint8_t* tiff = exif + 6;
                size_t tiff_len = seg_len - 6;

                bool big_endian = (tiff[0] == 'M' && tiff[1] == 'M');
                auto read16 = [big_endian, tiff](size_t off) -> uint16_t {
                    if (big_endian)
                        return (uint16_t)(tiff[off] << 8) | tiff[off + 1];
                    return (uint16_t)(tiff[off]) | ((uint16_t)(tiff[off + 1]) << 8);
                };

                if (tiff_len < 8) break;
                uint16_t ifd_offset_raw = read16(4);
                uint32_t ifd_offset = ifd_offset_raw | ((uint32_t)read16(6) << 16);
                if (big_endian) {
                    ifd_offset = ((uint32_t)read16(4) << 16) | read16(6);
                }

                if (ifd_offset + 2 > tiff_len) break;
                uint16_t entry_count = read16(ifd_offset);

                for (uint16_t i = 0; i < entry_count; i++) {
                    size_t entry_off = ifd_offset + 2 + i * 12;
                    if (entry_off + 12 > tiff_len) break;
                    uint16_t tag = read16(entry_off);
                    if (tag == 0x0112) {
                        uint16_t val = read16(entry_off + 8);
                        if (val >= 1 && val <= 8)
                            return static_cast<GopostExifOrientation>(val);
                    }
                }
            }
            break;
        }

        if (marker == 0xDA) break;

        uint16_t seg_len = (uint16_t)(data[pos + 2] << 8) | data[pos + 3];
        pos += 2 + seg_len;
    }

    return GOPOST_ORIENTATION_NORMAL;
}

#if defined(__APPLE__)

static GopostError decode_with_imageio(
    const uint8_t* data, size_t data_len,
    int32_t max_w, int32_t max_h,
    GopostFrame** out_frame
) {
    CFDataRef cf_data = CFDataCreate(kCFAllocatorDefault, data, (CFIndex)data_len);
    if (!cf_data) return GOPOST_ERROR_OUT_OF_MEMORY;

    CGImageSourceRef source = CGImageSourceCreateWithData(cf_data, nullptr);
    CFRelease(cf_data);
    if (!source) return GOPOST_ERROR_IO;

    CFDictionaryRef props = CGImageSourceCopyPropertiesAtIndex(source, 0, nullptr);
    int32_t img_w = 0, img_h = 0;
    if (props) {
        CFNumberRef w_ref = (CFNumberRef)CFDictionaryGetValue(props, kCGImagePropertyPixelWidth);
        CFNumberRef h_ref = (CFNumberRef)CFDictionaryGetValue(props, kCGImagePropertyPixelHeight);
        if (w_ref) CFNumberGetValue(w_ref, kCFNumberSInt32Type, &img_w);
        if (h_ref) CFNumberGetValue(h_ref, kCFNumberSInt32Type, &img_h);
        CFRelease(props);
    }

    int target_w = img_w, target_h = img_h;
    if (max_w > 0 && max_h > 0 && (img_w > max_w || img_h > max_h)) {
        float scale = std::min((float)max_w / img_w, (float)max_h / img_h);
        target_w = (int)(img_w * scale);
        target_h = (int)(img_h * scale);
    }

    CGImageRef cg_image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
    CFRelease(source);
    if (!cg_image) return GOPOST_ERROR_IO;

    size_t w = CGImageGetWidth(cg_image);
    size_t h = CGImageGetHeight(cg_image);

    if (max_w > 0 && max_h > 0 && ((int)w > max_w || (int)h > max_h)) {
        float scale = std::min((float)max_w / w, (float)max_h / h);
        w = (size_t)(w * scale);
        h = (size_t)(h * scale);
    }

    size_t stride = w * 4;
    size_t buf_size = stride * h;

    auto* frame = static_cast<GopostFrame*>(malloc(sizeof(GopostFrame)));
    if (!frame) {
        CGImageRelease(cg_image);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    frame->data = static_cast<uint8_t*>(malloc(buf_size));
    if (!frame->data) {
        free(frame);
        CGImageRelease(cg_image);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        frame->data, w, h, 8, stride,
        cs,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big
    );
    CGColorSpaceRelease(cs);

    if (!ctx) {
        free(frame->data);
        free(frame);
        CGImageRelease(cg_image);
        return GOPOST_ERROR_IO;
    }

    CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), cg_image);
    CGContextRelease(ctx);
    CGImageRelease(cg_image);

    /* Un-premultiply alpha for engine-internal RGBA representation */
    for (size_t i = 0; i < buf_size; i += 4) {
        uint8_t a = frame->data[i + 3];
        if (a > 0 && a < 255) {
            frame->data[i + 0] = (uint8_t)std::min(255, (int)frame->data[i + 0] * 255 / a);
            frame->data[i + 1] = (uint8_t)std::min(255, (int)frame->data[i + 1] * 255 / a);
            frame->data[i + 2] = (uint8_t)std::min(255, (int)frame->data[i + 2] * 255 / a);
        }
    }

    frame->width = (uint32_t)w;
    frame->height = (uint32_t)h;
    frame->format = GOPOST_PIXEL_FORMAT_RGBA8;
    frame->data_size = buf_size;
    frame->stride = stride;

    *out_frame = frame;
    return GOPOST_OK;
}

#else

static GopostError decode_fallback(
    const uint8_t* data, size_t data_len,
    int32_t max_w, int32_t max_h,
    GopostFrame** out_frame
) {
#if defined(GOPOST_HAS_STB_IMAGE)
    int w = 0, h = 0, channels = 0;
    uint8_t* pixels = stbi_load_from_memory(
        data, (int)data_len, &w, &h, &channels, 4 /* force RGBA */
    );
    if (!pixels) return GOPOST_ERROR_IO;

    if (max_w > 0 && max_h > 0 && (w > max_w || h > max_h)) {
        float scale = std::min((float)max_w / w, (float)max_h / h);
        int nw = (int)(w * scale), nh = (int)(h * scale);
        if (nw > 0 && nh > 0) {
            size_t new_size = (size_t)nw * nh * 4;
            auto* resized = static_cast<uint8_t*>(malloc(new_size));
            if (resized) {
                for (int dy = 0; dy < nh; dy++) {
                    int sy = dy * h / nh;
                    for (int dx = 0; dx < nw; dx++) {
                        int sx = dx * w / nw;
                        memcpy(resized + ((size_t)dy * nw + dx) * 4,
                               pixels + ((size_t)sy * w + sx) * 4, 4);
                    }
                }
                stbi_image_free(pixels);
                pixels = resized;
                w = nw;
                h = nh;
            }
        }
    }

    size_t stride = (size_t)w * 4;
    size_t buf_size = stride * h;

    auto* frame = static_cast<GopostFrame*>(malloc(sizeof(GopostFrame)));
    if (!frame) {
        stbi_image_free(pixels);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    frame->data = static_cast<uint8_t*>(malloc(buf_size));
    if (!frame->data) {
        free(frame);
        stbi_image_free(pixels);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    memcpy(frame->data, pixels, buf_size);
    stbi_image_free(pixels);

    frame->width = (uint32_t)w;
    frame->height = (uint32_t)h;
    frame->format = GOPOST_PIXEL_FORMAT_RGBA8;
    frame->data_size = buf_size;
    frame->stride = stride;

    *out_frame = frame;
    return GOPOST_OK;
#else
    (void)data; (void)data_len; (void)max_w; (void)max_h; (void)out_frame;
    return GOPOST_ERROR_CODEC_NOT_FOUND;
#endif
}

#endif

extern "C" {

GopostError gopost_image_probe(
    const uint8_t* data, size_t data_len,
    GopostImageInfo* out_info
) {
    if (!data || !out_info) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (data_len < 4) return GOPOST_ERROR_INVALID_ARGUMENT;

    memset(out_info, 0, sizeof(GopostImageInfo));
    out_info->format = detect_format(data, data_len);
    out_info->orientation = GOPOST_ORIENTATION_NORMAL;
    out_info->color_space = 0;

    if (out_info->format == GOPOST_IMAGE_FORMAT_JPEG) {
        out_info->orientation = read_jpeg_orientation(data, data_len);
        out_info->channels = 3;
        out_info->has_alpha = 0;

        /* Parse SOF for dimensions */
        size_t pos = 2;
        while (pos + 4 < data_len) {
            if (data[pos] != 0xFF) break;
            uint8_t m = data[pos + 1];
            if (m == 0xC0 || m == 0xC2) {
                if (pos + 9 < data_len) {
                    out_info->height = (data[pos + 5] << 8) | data[pos + 6];
                    out_info->width = (data[pos + 7] << 8) | data[pos + 8];
                }
                break;
            }
            if (m == 0xDA) break;
            uint16_t seg = (uint16_t)(data[pos + 2] << 8) | data[pos + 3];
            pos += 2 + seg;
        }
    } else if (out_info->format == GOPOST_IMAGE_FORMAT_PNG) {
        if (data_len >= 24) {
            out_info->width  = (data[16] << 24) | (data[17] << 16) | (data[18] << 8) | data[19];
            out_info->height = (data[20] << 24) | (data[21] << 16) | (data[22] << 8) | data[23];
            uint8_t color_type = (data_len > 25) ? data[25] : 0;
            out_info->has_alpha = (color_type == 4 || color_type == 6) ? 1 : 0;
            out_info->channels = out_info->has_alpha ? 4 : 3;
        }
    } else if (out_info->format == GOPOST_IMAGE_FORMAT_WEBP) {
        if (data_len >= 30 && memcmp(data + 12, "VP8 ", 4) == 0) {
            if (data_len >= 30) {
                out_info->width  = (data[26] | (data[27] << 8)) & 0x3FFF;
                out_info->height = (data[28] | (data[29] << 8)) & 0x3FFF;
            }
            out_info->channels = 3;
            out_info->has_alpha = 0;
        } else if (data_len >= 30 && memcmp(data + 12, "VP8L", 4) == 0) {
            if (data_len >= 25) {
                uint32_t bits = data[21] | (data[22] << 8) | (data[23] << 16) | (data[24] << 24);
                out_info->width  = (bits & 0x3FFF) + 1;
                out_info->height = ((bits >> 14) & 0x3FFF) + 1;
                out_info->has_alpha = (bits >> 28) & 1;
                out_info->channels = out_info->has_alpha ? 4 : 3;
            }
        }
    } else if (out_info->format == GOPOST_IMAGE_FORMAT_BMP) {
        if (data_len >= 26) {
            out_info->width  = (int32_t)(data[18] | (data[19] << 8) | (data[20] << 16) | (data[21] << 24));
            int32_t h = (int32_t)(data[22] | (data[23] << 8) | (data[24] << 16) | (data[25] << 24));
            out_info->height = h < 0 ? -h : h;
            uint16_t bpp = (data_len >= 30) ? (uint16_t)(data[28] | (data[29] << 8)) : 24;
            out_info->has_alpha = (bpp == 32) ? 1 : 0;
            out_info->channels = out_info->has_alpha ? 4 : 3;
        }
    } else if (out_info->format == GOPOST_IMAGE_FORMAT_GIF) {
        if (data_len >= 10) {
            out_info->width  = (int32_t)(data[6] | (data[7] << 8));
            out_info->height = (int32_t)(data[8] | (data[9] << 8));
            out_info->channels = 4;
            out_info->has_alpha = 1;
        }
    } else if (out_info->format == GOPOST_IMAGE_FORMAT_TIFF) {
        out_info->channels = 3;
        out_info->has_alpha = 0;
    }

    return GOPOST_OK;
}

GopostError gopost_image_decode(
    GopostEngine* engine,
    const uint8_t* data, size_t data_len,
    GopostFrame** out_frame
) {
    if (!engine || !data || !out_frame) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (data_len < 4) return GOPOST_ERROR_INVALID_ARGUMENT;

#if defined(__APPLE__)
    return decode_with_imageio(data, data_len, 0, 0, out_frame);
#else
    return decode_fallback(data, data_len, 0, 0, out_frame);
#endif
}

GopostError gopost_image_decode_file(
    GopostEngine* engine,
    const char* path,
    GopostFrame** out_frame
) {
    if (!engine || !path || !out_frame) return GOPOST_ERROR_INVALID_ARGUMENT;

    FILE* f = fopen(path, "rb");
    if (!f) return GOPOST_ERROR_IO;

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(f);
        return GOPOST_ERROR_IO;
    }

    auto* buf = static_cast<uint8_t*>(malloc((size_t)file_size));
    if (!buf) {
        fclose(f);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    size_t read = fread(buf, 1, (size_t)file_size, f);
    fclose(f);

    if (read != (size_t)file_size) {
        free(buf);
        return GOPOST_ERROR_IO;
    }

    GopostError err = gopost_image_decode(engine, buf, (size_t)file_size, out_frame);
    free(buf);
    return err;
}

GopostError gopost_image_decode_resize(
    GopostEngine* engine,
    const uint8_t* data, size_t data_len,
    int32_t max_width, int32_t max_height,
    GopostFrame** out_frame
) {
    if (!engine || !data || !out_frame) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (data_len < 4) return GOPOST_ERROR_INVALID_ARGUMENT;

#if defined(__APPLE__)
    return decode_with_imageio(data, data_len, max_width, max_height, out_frame);
#else
    return decode_fallback(data, data_len, max_width, max_height, out_frame);
#endif
}

} // extern "C"
