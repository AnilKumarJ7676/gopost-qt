#include <gtest/gtest.h>
#include "gopost/image_codec.h"

TEST(ImageCodec, ProbeRejectsNull) {
    GopostImageInfo info;
    EXPECT_EQ(gopost_image_probe(nullptr, 0, &info), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(ImageCodec, ProbeRejectsTooSmall) {
    uint8_t data[] = {0, 1, 2};
    GopostImageInfo info;
    EXPECT_EQ(gopost_image_probe(data, 3, &info), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(ImageCodec, ProbeDetectsJpeg) {
    uint8_t jpeg_header[] = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J', 'F'};
    GopostImageInfo info;
    EXPECT_EQ(gopost_image_probe(jpeg_header, sizeof(jpeg_header), &info), GOPOST_OK);
    EXPECT_EQ(info.format, GOPOST_IMAGE_FORMAT_JPEG);
}

TEST(ImageCodec, ProbeDetectsPng) {
    uint8_t png_header[] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    GopostImageInfo info;
    EXPECT_EQ(gopost_image_probe(png_header, sizeof(png_header), &info), GOPOST_OK);
    EXPECT_EQ(info.format, GOPOST_IMAGE_FORMAT_PNG);
}

TEST(ImageCodec, ProbeDetectsWebp) {
    uint8_t webp_header[] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'E', 'B', 'P'};
    GopostImageInfo info;
    EXPECT_EQ(gopost_image_probe(webp_header, sizeof(webp_header), &info), GOPOST_OK);
    EXPECT_EQ(info.format, GOPOST_IMAGE_FORMAT_WEBP);
}

TEST(ImageCodec, DecodeRejectsNull) {
    GopostFrame* frame = nullptr;
    EXPECT_EQ(gopost_image_decode(nullptr, nullptr, 0, &frame), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(ImageCodec, EncodeJpegRejectsNull) {
    uint8_t* data = nullptr;
    size_t size = 0;
    EXPECT_EQ(gopost_image_encode_jpeg(nullptr, nullptr, &data, &size), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(ImageCodec, EncodePngRejectsNull) {
    uint8_t* data = nullptr;
    size_t size = 0;
    EXPECT_EQ(gopost_image_encode_png(nullptr, nullptr, &data, &size), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(ImageCodec, EncodeWebpRejectsNull) {
    uint8_t* data = nullptr;
    size_t size = 0;
    EXPECT_EQ(gopost_image_encode_webp(nullptr, nullptr, &data, &size), GOPOST_ERROR_INVALID_ARGUMENT);
}
