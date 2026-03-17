#include "gopost/effects.h"
#include "gopost/types.h"

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>

extern "C" {

static void generate_gaussian_kernel(float radius, std::vector<float>& kernel) {
    int r = std::max(1, (int)std::ceil(radius));
    int size = 2 * r + 1;
    kernel.resize(size);
    float sigma = radius / 3.0f;
    if (sigma < 0.01f) sigma = 0.01f;
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        float x = (float)(i - r);
        kernel[i] = std::exp(-(x * x) / (2.0f * sigma * sigma));
        sum += kernel[i];
    }
    for (int i = 0; i < size; i++) kernel[i] /= sum;
}

GopostError gopost_effect_gaussian_blur(GopostFrame* frame, float radius) {
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;
    radius = std::clamp(radius, 0.0f, 100.0f);
    if (radius < 0.5f) return GOPOST_OK;

    int w = (int)frame->width;
    int h = (int)frame->height;
    int stride = w * 4;

    std::vector<float> kernel;
    generate_gaussian_kernel(radius, kernel);
    int r = (int)(kernel.size() / 2);

    std::vector<uint8_t> tmp(w * h * 4);

    // Horizontal pass
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float sum_r = 0, sum_g = 0, sum_b = 0;
            for (int k = -r; k <= r; k++) {
                int sx = std::clamp(x + k, 0, w - 1);
                int idx = y * stride + sx * 4;
                float weight = kernel[k + r];
                sum_r += frame->data[idx + 0] * weight;
                sum_g += frame->data[idx + 1] * weight;
                sum_b += frame->data[idx + 2] * weight;
            }
            int out_idx = y * stride + x * 4;
            tmp[out_idx + 0] = (uint8_t)std::clamp(sum_r, 0.0f, 255.0f);
            tmp[out_idx + 1] = (uint8_t)std::clamp(sum_g, 0.0f, 255.0f);
            tmp[out_idx + 2] = (uint8_t)std::clamp(sum_b, 0.0f, 255.0f);
            tmp[out_idx + 3] = frame->data[y * stride + x * 4 + 3];
        }
    }

    // Vertical pass
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float sum_r = 0, sum_g = 0, sum_b = 0;
            for (int k = -r; k <= r; k++) {
                int sy = std::clamp(y + k, 0, h - 1);
                int idx = sy * stride + x * 4;
                float weight = kernel[k + r];
                sum_r += tmp[idx + 0] * weight;
                sum_g += tmp[idx + 1] * weight;
                sum_b += tmp[idx + 2] * weight;
            }
            int out_idx = y * stride + x * 4;
            frame->data[out_idx + 0] = (uint8_t)std::clamp(sum_r, 0.0f, 255.0f);
            frame->data[out_idx + 1] = (uint8_t)std::clamp(sum_g, 0.0f, 255.0f);
            frame->data[out_idx + 2] = (uint8_t)std::clamp(sum_b, 0.0f, 255.0f);
        }
    }

    return GOPOST_OK;
}

GopostError gopost_effect_box_blur(GopostFrame* frame, float radius) {
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;
    radius = std::clamp(radius, 0.0f, 100.0f);
    if (radius < 0.5f) return GOPOST_OK;

    int w = (int)frame->width;
    int h = (int)frame->height;
    int stride = w * 4;
    int r = std::max(1, (int)std::round(radius));

    std::vector<uint8_t> tmp(w * h * 4);

    // Horizontal pass
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int sum_r = 0, sum_g = 0, sum_b = 0, count = 0;
            for (int k = -r; k <= r; k++) {
                int sx = std::clamp(x + k, 0, w - 1);
                int idx = y * stride + sx * 4;
                sum_r += frame->data[idx + 0];
                sum_g += frame->data[idx + 1];
                sum_b += frame->data[idx + 2];
                count++;
            }
            int out_idx = y * stride + x * 4;
            tmp[out_idx + 0] = (uint8_t)(sum_r / count);
            tmp[out_idx + 1] = (uint8_t)(sum_g / count);
            tmp[out_idx + 2] = (uint8_t)(sum_b / count);
            tmp[out_idx + 3] = frame->data[y * stride + x * 4 + 3];
        }
    }

    // Vertical pass
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int sum_r = 0, sum_g = 0, sum_b = 0, count = 0;
            for (int k = -r; k <= r; k++) {
                int sy = std::clamp(y + k, 0, h - 1);
                int idx = sy * stride + x * 4;
                sum_r += tmp[idx + 0];
                sum_g += tmp[idx + 1];
                sum_b += tmp[idx + 2];
                count++;
            }
            int out_idx = y * stride + x * 4;
            frame->data[out_idx + 0] = (uint8_t)(sum_r / count);
            frame->data[out_idx + 1] = (uint8_t)(sum_g / count);
            frame->data[out_idx + 2] = (uint8_t)(sum_b / count);
        }
    }

    return GOPOST_OK;
}

GopostError gopost_effect_unsharp_mask(
    GopostFrame* frame, float amount, float radius, float threshold
) {
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;

    int w = (int)frame->width;
    int h = (int)frame->height;
    size_t pixel_count = (size_t)w * h * 4;

    std::vector<uint8_t> blurred(pixel_count);
    memcpy(blurred.data(), frame->data, pixel_count);

    GopostFrame blur_frame{};
    blur_frame.width = frame->width;
    blur_frame.height = frame->height;
    blur_frame.format = frame->format;
    blur_frame.data = blurred.data();
    blur_frame.data_size = pixel_count;
    blur_frame.stride = frame->stride;

    GopostError err = gopost_effect_gaussian_blur(&blur_frame, radius);
    if (err != GOPOST_OK) return err;

    for (size_t i = 0; i < pixel_count; i += 4) {
        for (int c = 0; c < 3; c++) {
            float orig = frame->data[i + c];
            float blur_val = blurred[i + c];
            float diff = orig - blur_val;
            if (std::fabs(diff) > threshold) {
                float result = orig + amount * diff;
                frame->data[i + c] = (uint8_t)std::clamp(result, 0.0f, 255.0f);
            }
        }
    }

    return GOPOST_OK;
}

GopostError gopost_effect_radial_blur(
    GopostFrame* frame, float center_x, float center_y, float amount
) {
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;

    int w = (int)frame->width;
    int h = (int)frame->height;
    int stride = w * 4;
    int samples = std::clamp((int)(amount * 0.1f), 2, 20);

    std::vector<uint8_t> output(w * h * 4);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float dx = (float)x - center_x;
            float dy = (float)y - center_y;
            float dist = std::sqrt(dx * dx + dy * dy);
            float step = dist > 0 ? 1.0f / (float)samples : 0;

            float sum_r = 0, sum_g = 0, sum_b = 0;
            int count = 0;

            for (int s = 0; s < samples; s++) {
                float t = (float)s * step;
                int sx = std::clamp((int)(x - dx * t * 0.01f), 0, w - 1);
                int sy = std::clamp((int)(y - dy * t * 0.01f), 0, h - 1);
                int idx = sy * stride + sx * 4;
                sum_r += frame->data[idx + 0];
                sum_g += frame->data[idx + 1];
                sum_b += frame->data[idx + 2];
                count++;
            }

            int out_idx = y * stride + x * 4;
            output[out_idx + 0] = (uint8_t)(sum_r / count);
            output[out_idx + 1] = (uint8_t)(sum_g / count);
            output[out_idx + 2] = (uint8_t)(sum_b / count);
            output[out_idx + 3] = frame->data[out_idx + 3];
        }
    }

    memcpy(frame->data, output.data(), w * h * 4);
    return GOPOST_OK;
}

GopostError gopost_effect_tilt_shift(
    GopostFrame* frame, float focus_y, float blur_radius, float transition_size
) {
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (blur_radius < 0.5f) return GOPOST_OK;

    int w = (int)frame->width;
    int h = (int)frame->height;

    std::vector<uint8_t> original(w * h * 4);
    memcpy(original.data(), frame->data, w * h * 4);

    GopostError err = gopost_effect_gaussian_blur(frame, blur_radius);
    if (err != GOPOST_OK) return err;

    if (transition_size < 1.0f) transition_size = 1.0f;

    for (int y = 0; y < h; y++) {
        float dist = std::fabs((float)y - focus_y);
        float t = std::clamp((dist - transition_size * 0.5f) / transition_size, 0.0f, 1.0f);

        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 4;
            for (int c = 0; c < 3; c++) {
                float sharp = (float)original[idx + c];
                float blurred = (float)frame->data[idx + c];
                frame->data[idx + c] = (uint8_t)(sharp + t * (blurred - sharp));
            }
            frame->data[idx + 3] = original[idx + 3];
        }
    }

    return GOPOST_OK;
}

} // extern "C"
