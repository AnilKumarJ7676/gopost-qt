#include "gopost/image_codec.h"
#include "gopost/engine.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#if !defined(__APPLE__)
#ifdef __has_include
#if __has_include("stb_image_write.h")
#include "stb_image_write.h"
#define GOPOST_HAS_STB_IMAGE_WRITE 1
#endif
#endif
#endif

#if defined(GOPOST_HAS_WEBP)
#include <webp/encode.h>
#endif

#if defined(__APPLE__)

struct EncoderBuffer {
    uint8_t* data;
    size_t size;
    size_t capacity;
};

static size_t buffer_consumer(void* info, const void* buffer, size_t count) {
    auto* eb = static_cast<EncoderBuffer*>(info);
    size_t needed = eb->size + count;
    if (needed > eb->capacity) {
        size_t new_cap = eb->capacity * 2;
        if (new_cap < needed) new_cap = needed;
        auto* tmp = static_cast<uint8_t*>(realloc(eb->data, new_cap));
        if (!tmp) return 0;
        eb->data = tmp;
        eb->capacity = new_cap;
    }
    memcpy(eb->data + eb->size, buffer, count);
    eb->size += count;
    return count;
}

static GopostError encode_with_imageio(
    GopostFrame* frame,
    CFStringRef uti_type,
    float quality,
    uint8_t** out_data, size_t* out_size
) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        frame->data, frame->width, frame->height, 8, frame->stride,
        cs,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big
    );
    CGColorSpaceRelease(cs);
    if (!ctx) return GOPOST_ERROR_IO;

    CGImageRef cg_image = CGBitmapContextCreateImage(ctx);
    CGContextRelease(ctx);
    if (!cg_image) return GOPOST_ERROR_IO;

    EncoderBuffer eb = {nullptr, 0, 0};
    eb.capacity = frame->width * frame->height * 4;
    eb.data = static_cast<uint8_t*>(malloc(eb.capacity));
    if (!eb.data) {
        CGImageRelease(cg_image);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    CGDataConsumerCallbacks callbacks = { buffer_consumer, nullptr };
    CGDataConsumerRef consumer = CGDataConsumerCreate(&eb, &callbacks);
    if (!consumer) {
        free(eb.data);
        CGImageRelease(cg_image);
        return GOPOST_ERROR_IO;
    }

    CGImageDestinationRef dest = CGImageDestinationCreateWithDataConsumer(
        consumer, uti_type, 1, nullptr
    );
    CGDataConsumerRelease(consumer);

    if (!dest) {
        free(eb.data);
        CGImageRelease(cg_image);
        return GOPOST_ERROR_IO;
    }

    CFMutableDictionaryRef opts = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 1,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    CFNumberRef quality_num = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloatType, &quality);
    CFDictionarySetValue(opts, kCGImageDestinationLossyCompressionQuality, quality_num);
    CFRelease(quality_num);

    CGImageDestinationAddImage(dest, cg_image, opts);
    CFRelease(opts);

    bool ok = CGImageDestinationFinalize(dest);
    CFRelease(dest);
    CGImageRelease(cg_image);

    if (!ok) {
        free(eb.data);
        return GOPOST_ERROR_IO;
    }

    *out_data = eb.data;
    *out_size = eb.size;
    return GOPOST_OK;
}

#endif

extern "C" {

#if !defined(__APPLE__) && defined(GOPOST_HAS_STB_IMAGE_WRITE)
struct StbWriteContext {
    uint8_t* data;
    size_t size;
    size_t capacity;
};

static void stb_write_func(void* ctx, void* data, int size) {
    auto* wc = static_cast<StbWriteContext*>(ctx);
    size_t needed = wc->size + (size_t)size;
    if (needed > wc->capacity) {
        size_t new_cap = wc->capacity * 2;
        if (new_cap < needed) new_cap = needed;
        auto* tmp = static_cast<uint8_t*>(realloc(wc->data, new_cap));
        if (!tmp) return;
        wc->data = tmp;
        wc->capacity = new_cap;
    }
    memcpy(wc->data + wc->size, data, (size_t)size);
    wc->size += (size_t)size;
}
#endif

GopostError gopost_image_encode_jpeg(
    GopostFrame* frame,
    const GopostJpegEncodeOpts* opts,
    uint8_t** out_data, size_t* out_size
) {
    if (!frame || !out_data || !out_size) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;

    int quality = (opts && opts->quality > 0 && opts->quality <= 100) ? opts->quality : 85;

#if defined(__APPLE__)
    CFStringRef uti = CFSTR("public.jpeg");
    return encode_with_imageio(frame, uti, quality / 100.0f, out_data, out_size);
#elif defined(GOPOST_HAS_STB_IMAGE_WRITE)
    StbWriteContext wc = {nullptr, 0, (size_t)frame->width * frame->height * 3};
    wc.data = static_cast<uint8_t*>(malloc(wc.capacity));
    if (!wc.data) return GOPOST_ERROR_OUT_OF_MEMORY;

    int ok = stbi_write_jpg_to_func(stb_write_func, &wc,
        (int)frame->width, (int)frame->height, 4, frame->data, quality);
    if (!ok || !wc.data) { free(wc.data); return GOPOST_ERROR_IO; }

    *out_data = wc.data;
    *out_size = wc.size;
    return GOPOST_OK;
#else
    (void)quality;
    return GOPOST_ERROR_CODEC_NOT_FOUND;
#endif
}

GopostError gopost_image_encode_png(
    GopostFrame* frame,
    const GopostPngEncodeOpts* opts,
    uint8_t** out_data, size_t* out_size
) {
    if (!frame || !out_data || !out_size) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;

    (void)opts;

#if defined(__APPLE__)
    CFStringRef uti = CFSTR("public.png");
    return encode_with_imageio(frame, uti, 1.0f, out_data, out_size);
#elif defined(GOPOST_HAS_STB_IMAGE_WRITE)
    StbWriteContext wc = {nullptr, 0, (size_t)frame->width * frame->height * 4};
    wc.data = static_cast<uint8_t*>(malloc(wc.capacity));
    if (!wc.data) return GOPOST_ERROR_OUT_OF_MEMORY;

    int stride = (int)(frame->width * 4);
    int ok = stbi_write_png_to_func(stb_write_func, &wc,
        (int)frame->width, (int)frame->height, 4, frame->data, stride);
    if (!ok || !wc.data) { free(wc.data); return GOPOST_ERROR_IO; }

    *out_data = wc.data;
    *out_size = wc.size;
    return GOPOST_OK;
#else
    return GOPOST_ERROR_CODEC_NOT_FOUND;
#endif
}

GopostError gopost_image_encode_webp(
    GopostFrame* frame,
    const GopostWebpEncodeOpts* opts,
    uint8_t** out_data, size_t* out_size
) {
    if (!frame || !out_data || !out_size) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;

    int quality = (opts && opts->quality > 0 && opts->quality <= 100) ? opts->quality : 80;

#if defined(__APPLE__)
    CFStringRef uti = CFSTR("org.webmproject.webp");
    return encode_with_imageio(frame, uti, quality / 100.0f, out_data, out_size);
#elif defined(GOPOST_HAS_WEBP)
    uint8_t* webp_data = nullptr;
    size_t webp_size = WebPEncodeRGBA(
        frame->data, (int)frame->width, (int)frame->height,
        (int)frame->stride, (float)quality, &webp_data);

    if (webp_size == 0 || !webp_data) return GOPOST_ERROR_IO;

    *out_data = static_cast<uint8_t*>(malloc(webp_size));
    if (!*out_data) {
        WebPFree(webp_data);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }
    memcpy(*out_data, webp_data, webp_size);
    *out_size = webp_size;
    WebPFree(webp_data);
    return GOPOST_OK;
#else
    (void)quality;
    return GOPOST_ERROR_CODEC_NOT_FOUND;
#endif
}

GopostError gopost_image_encode_heic(
    GopostFrame* frame,
    const GopostHeicEncodeOpts* opts,
    uint8_t** out_data, size_t* out_size
) {
    if (!frame || !out_data || !out_size) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;

#if defined(__APPLE__)
    int quality = (opts && opts->quality > 0 && opts->quality <= 100) ? opts->quality : 80;
    CFStringRef uti = CFSTR("public.heic");
    return encode_with_imageio(frame, uti, quality / 100.0f, out_data, out_size);
#else
    (void)opts;
    return GOPOST_ERROR_CODEC_NOT_FOUND;
#endif
}

GopostError gopost_image_encode_bmp(
    GopostFrame* frame,
    uint8_t** out_data, size_t* out_size
) {
    if (!frame || !out_data || !out_size) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;

#if defined(__APPLE__)
    CFStringRef uti = CFSTR("com.microsoft.bmp");
    return encode_with_imageio(frame, uti, 1.0f, out_data, out_size);
#elif defined(GOPOST_HAS_STB_IMAGE_WRITE)
    StbWriteContext wc = {nullptr, 0, (size_t)frame->width * frame->height * 4 + 54};
    wc.data = static_cast<uint8_t*>(malloc(wc.capacity));
    if (!wc.data) return GOPOST_ERROR_OUT_OF_MEMORY;

    int ok = stbi_write_bmp_to_func(stb_write_func, &wc,
        (int)frame->width, (int)frame->height, 4, frame->data);
    if (!ok || !wc.data) { free(wc.data); return GOPOST_ERROR_IO; }

    *out_data = wc.data;
    *out_size = wc.size;
    return GOPOST_OK;
#else
    return GOPOST_ERROR_CODEC_NOT_FOUND;
#endif
}

GopostError gopost_image_encode_to_file(
    GopostFrame* frame,
    const char* path,
    GopostImageFormat format,
    int32_t quality
) {
    if (!frame || !path) return GOPOST_ERROR_INVALID_ARGUMENT;

    uint8_t* encoded = nullptr;
    size_t encoded_size = 0;
    GopostError err;

    switch (format) {
        case GOPOST_IMAGE_FORMAT_JPEG: {
            GopostJpegEncodeOpts opts = { quality, 0 };
            err = gopost_image_encode_jpeg(frame, &opts, &encoded, &encoded_size);
            break;
        }
        case GOPOST_IMAGE_FORMAT_PNG: {
            GopostPngEncodeOpts opts = { 6 };
            err = gopost_image_encode_png(frame, &opts, &encoded, &encoded_size);
            break;
        }
        case GOPOST_IMAGE_FORMAT_WEBP: {
            GopostWebpEncodeOpts opts = { quality, 0 };
            err = gopost_image_encode_webp(frame, &opts, &encoded, &encoded_size);
            break;
        }
        case GOPOST_IMAGE_FORMAT_HEIC: {
            GopostHeicEncodeOpts opts = { quality };
            err = gopost_image_encode_heic(frame, &opts, &encoded, &encoded_size);
            break;
        }
        case GOPOST_IMAGE_FORMAT_BMP: {
            err = gopost_image_encode_bmp(frame, &encoded, &encoded_size);
            break;
        }
        default:
            return GOPOST_ERROR_INVALID_ARGUMENT;
    }

    if (err != GOPOST_OK) return err;

    FILE* f = fopen(path, "wb");
    if (!f) {
        gopost_image_encode_free(encoded);
        return GOPOST_ERROR_IO;
    }

    size_t written = fwrite(encoded, 1, encoded_size, f);
    fclose(f);
    gopost_image_encode_free(encoded);

    return (written == encoded_size) ? GOPOST_OK : GOPOST_ERROR_IO;
}

void gopost_image_encode_free(uint8_t* data) {
    free(data);
}

} // extern "C"
