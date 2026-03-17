#include "gopost/effects.h"
#include "gopost/engine.h"

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <random>

static inline uint8_t clamp8(float v) {
    int i = (int)(v + 0.5f);
    return (uint8_t)(i < 0 ? 0 : (i > 255 ? 255 : i));
}

/*
 * Kuwahara-style oil paint filter.
 * For each pixel, the surrounding area is divided into 4 quadrants.
 * The quadrant with the lowest variance determines the output color,
 * producing brush-stroke-like smoothing.
 */
static void apply_oil_paint(uint8_t* data, int w, int h, int radius) {
    if (radius < 1) radius = 1;
    if (radius > 20) radius = 20;

    const size_t sz = (size_t)w * h * 4;
    auto* tmp = static_cast<uint8_t*>(malloc(sz));
    if (!tmp) return;
    memcpy(tmp, data, sz);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float bestVar = 1e30f;
            float bestR = 0, bestG = 0, bestB = 0;

            int quadX[4][2] = {{x - radius, x}, {x, x + radius},
                               {x - radius, x}, {x, x + radius}};
            int quadY[4][2] = {{y - radius, y}, {y - radius, y},
                               {y, y + radius}, {y, y + radius}};

            for (int q = 0; q < 4; q++) {
                float sumR = 0, sumG = 0, sumB = 0;
                float sumR2 = 0, sumG2 = 0, sumB2 = 0;
                int cnt = 0;

                for (int qy = quadY[q][0]; qy <= quadY[q][1]; qy++) {
                    if (qy < 0 || qy >= h) continue;
                    for (int qx = quadX[q][0]; qx <= quadX[q][1]; qx++) {
                        if (qx < 0 || qx >= w) continue;
                        size_t off = ((size_t)qy * w + qx) * 4;
                        float r = tmp[off], g = tmp[off + 1], b = tmp[off + 2];
                        sumR += r; sumG += g; sumB += b;
                        sumR2 += r * r; sumG2 += g * g; sumB2 += b * b;
                        cnt++;
                    }
                }

                if (cnt == 0) continue;
                float invN = 1.0f / cnt;
                float meanR = sumR * invN, meanG = sumG * invN, meanB = sumB * invN;
                float var = (sumR2 * invN - meanR * meanR) +
                            (sumG2 * invN - meanG * meanG) +
                            (sumB2 * invN - meanB * meanB);

                if (var < bestVar) {
                    bestVar = var;
                    bestR = meanR; bestG = meanG; bestB = meanB;
                }
            }

            size_t off = ((size_t)y * w + x) * 4;
            data[off]     = clamp8(bestR);
            data[off + 1] = clamp8(bestG);
            data[off + 2] = clamp8(bestB);
        }
    }
    free(tmp);
}

/*
 * Watercolor effect: median-like smoothing + edge darkening + saturation boost.
 * Uses an iterative mean filter with variance-weighted blending and luminance-
 * based edge detection to simulate watercolor bleed.
 */
static void apply_watercolor(uint8_t* data, int w, int h, int brush_size, float wetness) {
    if (brush_size < 1) brush_size = 1;
    if (brush_size > 15) brush_size = 15;
    wetness = std::clamp(wetness, 0.0f, 1.0f);

    const size_t sz = (size_t)w * h * 4;
    auto* tmp = static_cast<uint8_t*>(malloc(sz));
    if (!tmp) return;
    memcpy(tmp, data, sz);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float sumR = 0, sumG = 0, sumB = 0;
            int cnt = 0;

            for (int dy = -brush_size; dy <= brush_size; dy++) {
                int ny = y + dy;
                if (ny < 0 || ny >= h) continue;
                for (int dx = -brush_size; dx <= brush_size; dx++) {
                    int nx = x + dx;
                    if (nx < 0 || nx >= w) continue;
                    if (dx * dx + dy * dy > brush_size * brush_size) continue;
                    size_t off = ((size_t)ny * w + nx) * 4;
                    sumR += tmp[off]; sumG += tmp[off + 1]; sumB += tmp[off + 2];
                    cnt++;
                }
            }

            if (cnt > 0) {
                float invN = 1.0f / cnt;
                float avgR = sumR * invN, avgG = sumG * invN, avgB = sumB * invN;

                size_t off = ((size_t)y * w + x) * 4;
                float origR = tmp[off], origG = tmp[off + 1], origB = tmp[off + 2];

                float edgeR = 0, edgeG = 0, edgeB = 0;
                int sobelCnt = 0;
                for (int dy2 = -1; dy2 <= 1; dy2++) {
                    int sy = y + dy2;
                    if (sy < 0 || sy >= h) continue;
                    for (int dx2 = -1; dx2 <= 1; dx2++) {
                        if (dx2 == 0 && dy2 == 0) continue;
                        int sx = x + dx2;
                        if (sx < 0 || sx >= w) continue;
                        size_t soff = ((size_t)sy * w + sx) * 4;
                        edgeR += fabsf(tmp[soff] - origR);
                        edgeG += fabsf(tmp[soff + 1] - origG);
                        edgeB += fabsf(tmp[soff + 2] - origB);
                        sobelCnt++;
                    }
                }
                float edgeMag = sobelCnt > 0
                    ? (edgeR + edgeG + edgeB) / (sobelCnt * 3.0f * 255.0f)
                    : 0.0f;
                float darken = 1.0f - edgeMag * wetness * 0.5f;

                float r = avgR * wetness + origR * (1.0f - wetness);
                float g = avgG * wetness + origG * (1.0f - wetness);
                float b = avgB * wetness + origB * (1.0f - wetness);
                data[off]     = clamp8(r * darken);
                data[off + 1] = clamp8(g * darken);
                data[off + 2] = clamp8(b * darken);
            }
        }
    }
    free(tmp);
}

/*
 * Sketch (pencil) effect: Sobel edge detection inverted on a white background,
 * producing a hand-drawn pencil sketch appearance.
 */
static void apply_sketch(uint8_t* data, int w, int h, float threshold, float darkness) {
    threshold = std::clamp(threshold, 0.0f, 1.0f) * 255.0f;
    darkness = std::clamp(darkness, 0.0f, 1.0f);

    const size_t sz = (size_t)w * h * 4;
    auto* tmp = static_cast<uint8_t*>(malloc(sz));
    if (!tmp) return;
    memcpy(tmp, data, sz);

    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            float gx = 0, gy = 0;
            static const int kx[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};
            static const int ky[3][3] = {{-1,-2,-1},{0,0,0},{1,2,1}};

            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    size_t off = ((size_t)(y + dy) * w + (x + dx)) * 4;
                    float lum = 0.299f * tmp[off] + 0.587f * tmp[off + 1] + 0.114f * tmp[off + 2];
                    gx += lum * kx[dy + 1][dx + 1];
                    gy += lum * ky[dy + 1][dx + 1];
                }
            }

            float mag = sqrtf(gx * gx + gy * gy);
            float edge = mag > threshold ? 1.0f : mag / std::max(threshold, 1.0f);
            float val = 255.0f * (1.0f - edge * darkness);

            size_t off = ((size_t)y * w + x) * 4;
            data[off]     = clamp8(val);
            data[off + 1] = clamp8(val);
            data[off + 2] = clamp8(val);
        }
    }
    free(tmp);
}

/*
 * Pixelate filter: downsamples in blocks of `block_size`, averaging
 * each block to a single color, producing a mosaic effect.
 */
static void apply_pixelate(uint8_t* data, int w, int h, int block_size) {
    if (block_size < 2) block_size = 2;
    if (block_size > 100) block_size = 100;

    for (int by = 0; by < h; by += block_size) {
        for (int bx = 0; bx < w; bx += block_size) {
            float sumR = 0, sumG = 0, sumB = 0, sumA = 0;
            int cnt = 0;
            int endY = std::min(by + block_size, h);
            int endX = std::min(bx + block_size, w);

            for (int y = by; y < endY; y++) {
                for (int x = bx; x < endX; x++) {
                    size_t off = ((size_t)y * w + x) * 4;
                    sumR += data[off]; sumG += data[off + 1];
                    sumB += data[off + 2]; sumA += data[off + 3];
                    cnt++;
                }
            }

            float invN = 1.0f / cnt;
            uint8_t avgR = clamp8(sumR * invN);
            uint8_t avgG = clamp8(sumG * invN);
            uint8_t avgB = clamp8(sumB * invN);
            uint8_t avgA = clamp8(sumA * invN);

            for (int y = by; y < endY; y++) {
                for (int x = bx; x < endX; x++) {
                    size_t off = ((size_t)y * w + x) * 4;
                    data[off] = avgR; data[off + 1] = avgG;
                    data[off + 2] = avgB; data[off + 3] = avgA;
                }
            }
        }
    }
}

/*
 * Glitch effect: horizontal RGB channel offset with random scanline
 * displacement. Uses a deterministic seed for reproducibility.
 */
static void apply_glitch(uint8_t* data, int w, int h, float amount, float seed) {
    amount = std::clamp(amount, 0.0f, 1.0f);
    if (amount < 0.01f) return;

    const size_t sz = (size_t)w * h * 4;
    auto* tmp = static_cast<uint8_t*>(malloc(sz));
    if (!tmp) return;
    memcpy(tmp, data, sz);

    int maxShift = (int)(amount * w * 0.1f);
    if (maxShift < 1) maxShift = 1;

    std::mt19937 rng((uint32_t)(seed * 65535.0f));
    std::uniform_int_distribution<int> shiftDist(-maxShift, maxShift);
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);

    for (int y = 0; y < h; y++) {
        bool doGlitch = probDist(rng) < amount * 0.4f;
        int rShift = doGlitch ? shiftDist(rng) : 0;
        int gShift = 0;
        int bShift = doGlitch ? shiftDist(rng) : 0;

        for (int x = 0; x < w; x++) {
            size_t dstOff = ((size_t)y * w + x) * 4;

            int rx = std::clamp(x + rShift, 0, w - 1);
            int bx = std::clamp(x + bShift, 0, w - 1);

            data[dstOff]     = tmp[((size_t)y * w + rx) * 4];
            data[dstOff + 1] = tmp[((size_t)y * w + x)  * 4 + 1];
            data[dstOff + 2] = tmp[((size_t)y * w + bx) * 4 + 2];
        }

        if (doGlitch && probDist(rng) < 0.3f) {
            int blockH = std::min(2 + (int)(amount * 8), h - y);
            int shift = shiftDist(rng) * 2;
            for (int dy = 0; dy < blockH && y + dy < h; dy++) {
                for (int x = 0; x < w; x++) {
                    int sx = std::clamp(x + shift, 0, w - 1);
                    size_t dOff = ((size_t)(y + dy) * w + x) * 4;
                    size_t sOff = ((size_t)(y + dy) * w + sx) * 4;
                    data[dOff]     = tmp[sOff];
                    data[dOff + 1] = tmp[sOff + 1];
                    data[dOff + 2] = tmp[sOff + 2];
                }
            }
        }
    }
    free(tmp);
}

/*
 * Halftone: simulates CMYK print halftone screens by computing dot coverage
 * at configurable size, angle, and contrast. Each channel uses a slightly
 * rotated grid to produce a classic rosette pattern.
 */
static void apply_halftone(uint8_t* data, int w, int h,
                           float dot_size, float angle_deg, float contrast) {
    dot_size = std::clamp(dot_size, 2.0f, 40.0f);
    contrast = std::clamp(contrast, 0.0f, 2.0f);
    const float pi = 3.14159265f;
    const float base_angle = angle_deg * pi / 180.0f;
    const float channel_offsets[3] = {0.0f, pi / 6.0f, pi / 3.0f};

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            size_t off = ((size_t)y * w + x) * 4;
            uint8_t result[3];

            for (int c = 0; c < 3; c++) {
                float a = base_angle + channel_offsets[c];
                float cos_a = cosf(a), sin_a = sinf(a);

                float rx = cos_a * x + sin_a * y;
                float ry = -sin_a * x + cos_a * y;

                float gx = rx / dot_size;
                float gy = ry / dot_size;
                float cx = (floorf(gx) + 0.5f) * dot_size;
                float cy = (floorf(gy) + 0.5f) * dot_size;

                float ux = cos_a * cx - sin_a * cy;
                float uy = sin_a * cx + cos_a * cy;

                float dx = (float)x - ux;
                float dy = (float)y - uy;
                float dist = sqrtf(dx * dx + dy * dy);

                float lum = data[off + c] / 255.0f;
                float max_radius = dot_size * 0.5f * sqrtf(lum) * contrast;

                float val = dist < max_radius ? 1.0f : 0.0f;
                float edge = dot_size * 0.05f;
                if (edge > 0.5f && dist >= max_radius - edge && dist < max_radius + edge) {
                    val = 1.0f - (dist - (max_radius - edge)) / (2.0f * edge);
                    val = std::clamp(val, 0.0f, 1.0f);
                }
                result[c] = clamp8(val * 255.0f);
            }

            data[off]     = result[0];
            data[off + 1] = result[1];
            data[off + 2] = result[2];
        }
    }
}

extern "C" {

GopostError gopost_effect_oil_paint(GopostFrame* frame, float radius, float intensity) {
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;
    int r = (int)(std::clamp(intensity, 0.0f, 100.0f) / 100.0f * 10.0f + radius);
    apply_oil_paint(frame->data, frame->width, frame->height, r);
    return GOPOST_OK;
}

GopostError gopost_effect_watercolor(GopostFrame* frame, float brush_size, float intensity) {
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;
    int bs = (int)(std::clamp(brush_size, 1.0f, 15.0f));
    float wet = std::clamp(intensity, 0.0f, 100.0f) / 100.0f;
    apply_watercolor(frame->data, frame->width, frame->height, bs, wet);
    return GOPOST_OK;
}

GopostError gopost_effect_sketch(GopostFrame* frame, float threshold, float intensity) {
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;
    float t = std::clamp(threshold, 0.0f, 100.0f) / 100.0f;
    float d = std::clamp(intensity, 0.0f, 100.0f) / 100.0f;
    apply_sketch(frame->data, frame->width, frame->height, t, d);
    return GOPOST_OK;
}

GopostError gopost_effect_pixelate(GopostFrame* frame, int block_size) {
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;
    apply_pixelate(frame->data, frame->width, frame->height, block_size);
    return GOPOST_OK;
}

GopostError gopost_effect_glitch(GopostFrame* frame, float amount, float seed) {
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;
    apply_glitch(frame->data, frame->width, frame->height, amount, seed);
    return GOPOST_OK;
}

GopostError gopost_effect_halftone(GopostFrame* frame, float dot_size, float angle, float contrast) {
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;
    apply_halftone(frame->data, frame->width, frame->height, dot_size, angle, contrast);
    return GOPOST_OK;
}

// Non-Apple stub — Metal shader source only available on Apple platforms
#if !defined(__APPLE__)
const char* gopost_artistic_compute_shader_source(void) {
    return nullptr;
}
#endif

} // extern "C"
