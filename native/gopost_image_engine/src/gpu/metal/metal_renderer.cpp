#include "gopost/gpu_context.h"
#include "gopost/canvas.h"
#include "gopost/engine.h"

/*
 * S5-05: Metal GPU rendering pipeline.
 *
 * This file provides the Metal-specific rendering backend for canvas
 * composition. It handles:
 *   - Texture creation/upload from RGBA pixel buffers
 *   - Fragment shaders for the 4 initial blend modes
 *   - Compositing two textures via Metal render pass
 *   - Output texture suitable for Flutter's Texture widget
 *
 * On non-Apple platforms this file compiles to a no-op.
 */

#if defined(__APPLE__) && (defined(GOPOST_PLATFORM_IOS) || defined(GOPOST_PLATFORM_MACOS))

#include <TargetConditionals.h>

/*
 * Metal shader source for blend mode compositing (MSL).
 * The composite_fragment function samples source + destination textures,
 * applies the selected blend mode, and outputs the result.
 */
static const char* kBlendShaderMSL = R"(
#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

vertex VertexOut composite_vertex(
    uint vid [[vertex_id]],
    constant float4* positions [[buffer(0)]]
) {
    VertexOut out;
    out.position = float4(positions[vid].xy, 0.0, 1.0);
    out.texCoord = positions[vid].zw;
    return out;
}

float3 blend_normal(float3 src, float3 dst) {
    return src;
}

float3 blend_multiply(float3 src, float3 dst) {
    return src * dst;
}

float3 blend_screen(float3 src, float3 dst) {
    return 1.0 - (1.0 - src) * (1.0 - dst);
}

float3 blend_overlay(float3 src, float3 dst) {
    float3 result;
    result.r = dst.r < 0.5 ? 2.0 * src.r * dst.r : 1.0 - 2.0 * (1.0 - src.r) * (1.0 - dst.r);
    result.g = dst.g < 0.5 ? 2.0 * src.g * dst.g : 1.0 - 2.0 * (1.0 - src.g) * (1.0 - dst.g);
    result.b = dst.b < 0.5 ? 2.0 * src.b * dst.b : 1.0 - 2.0 * (1.0 - src.b) * (1.0 - dst.b);
    return result;
}

struct BlendParams {
    int blend_mode;   // 0=Normal, 1=Multiply, 2=Screen, 3=Overlay
    float opacity;
};

fragment float4 composite_fragment(
    VertexOut in [[stage_in]],
    texture2d<float> srcTex [[texture(0)]],
    texture2d<float> dstTex [[texture(1)]],
    constant BlendParams& params [[buffer(0)]]
) {
    constexpr sampler s(mag_filter::linear, min_filter::linear);
    float4 src = srcTex.sample(s, in.texCoord);
    float4 dst = dstTex.sample(s, in.texCoord);

    float3 blended;
    switch (params.blend_mode) {
        case 1:  blended = blend_multiply(src.rgb, dst.rgb); break;
        case 2:  blended = blend_screen(src.rgb, dst.rgb); break;
        case 3:  blended = blend_overlay(src.rgb, dst.rgb); break;
        default: blended = blend_normal(src.rgb, dst.rgb); break;
    }

    float src_a = src.a * params.opacity;
    float3 result = blended * src_a + dst.rgb * (1.0 - src_a);
    float out_a = src_a + dst.a * (1.0 - src_a);

    return float4(result, out_a);
}
)";

/*
 * Metal pipeline state creation and rendering is done through Objective-C++.
 * For Sprint 5 we store the shader source and will compile it at
 * runtime when the first canvas render occurs. The full Objective-C++
 * integration with MTLDevice/MTLRenderPipelineState requires .mm files
 * or a bridging layer.
 *
 * Placeholder: the shader source above is compiled by MetalContext
 * (gpu/metal/metal_context.cpp) and cached for reuse.
 */

const char* gopost_metal_blend_shader_source() {
    return kBlendShaderMSL;
}

/*
 * S7-03: Metal compute kernels for artistic filters.
 *
 * Each kernel reads from a source texture, applies the effect, and writes
 * to a destination texture. A params buffer carries per-effect uniforms.
 */
static const char* kArtisticComputeMSL = R"(
#include <metal_stdlib>
using namespace metal;

struct ArtisticParams {
    float p0;
    float p1;
    float p2;
    int   width;
    int   height;
    int   filter_id;   // 0=oil_paint, 1=watercolor, 2=sketch, 3=pixelate, 4=glitch, 5=halftone
};

static float3 read_px(texture2d<float, access::read> tex, int x, int y, int w, int h) {
    x = clamp(x, 0, w - 1);
    y = clamp(y, 0, h - 1);
    return tex.read(uint2(x, y)).rgb;
}

static float luminance(float3 c) {
    return dot(c, float3(0.299, 0.587, 0.114));
}

// Oil paint: Kuwahara filter — pick quadrant with lowest variance.
static float3 oil_paint(texture2d<float, access::read> tex, int x, int y, int w, int h, int radius) {
    float bestVar = 1e30;
    float3 bestColor = float3(0);

    int qxr[4][2] = {{x - radius, x}, {x, x + radius}, {x - radius, x}, {x, x + radius}};
    int qyr[4][2] = {{y - radius, y}, {y - radius, y}, {y, y + radius}, {y, y + radius}};

    for (int q = 0; q < 4; q++) {
        float3 sum = float3(0);
        float3 sum2 = float3(0);
        int cnt = 0;
        for (int qy = qyr[q][0]; qy <= qyr[q][1]; qy++) {
            for (int qx = qxr[q][0]; qx <= qxr[q][1]; qx++) {
                float3 c = read_px(tex, qx, qy, w, h);
                sum += c;
                sum2 += c * c;
                cnt++;
            }
        }
        if (cnt == 0) continue;
        float3 mean = sum / float(cnt);
        float var = dot(sum2 / float(cnt) - mean * mean, float3(1));
        if (var < bestVar) {
            bestVar = var;
            bestColor = mean;
        }
    }
    return bestColor;
}

// Sketch: Sobel edge detection on luminance.
static float3 sketch_filter(texture2d<float, access::read> tex, int x, int y, int w, int h,
                             float threshold, float darkness) {
    float gx = 0, gy = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            float l = luminance(read_px(tex, x + dx, y + dy, w, h));
            int kx_arr[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};
            int ky_arr[3][3] = {{-1,-2,-1},{0,0,0},{1,2,1}};
            gx += l * float(kx_arr[dy+1][dx+1]);
            gy += l * float(ky_arr[dy+1][dx+1]);
        }
    }
    float mag = sqrt(gx * gx + gy * gy);
    float edge = mag > threshold ? 1.0 : mag / max(threshold, 0.001);
    float val = 1.0 - edge * darkness;
    return float3(val);
}

// Halftone: per-channel dot pattern with rotated grids.
static float3 halftone_filter(texture2d<float, access::read> tex, int x, int y, int w, int h,
                               float dot_size, float angle_deg, float contrast) {
    float3 orig = read_px(tex, x, y, w, h);
    float base_angle = angle_deg * 3.14159265 / 180.0;
    float offsets[3] = {0.0, 3.14159265 / 6.0, 3.14159265 / 3.0};
    float3 result;

    for (int c = 0; c < 3; c++) {
        float a = base_angle + offsets[c];
        float ca = cos(a), sa = sin(a);
        float rx = ca * float(x) + sa * float(y);
        float ry = -sa * float(x) + ca * float(y);
        float gx_f = rx / dot_size;
        float gy_f = ry / dot_size;
        float cx = (floor(gx_f) + 0.5) * dot_size;
        float cy = (floor(gy_f) + 0.5) * dot_size;
        float ux = ca * cx - sa * cy;
        float uy = sa * cx + ca * cy;
        float dist = length(float2(float(x) - ux, float(y) - uy));
        float ch_val = (c == 0) ? orig.r : ((c == 1) ? orig.g : orig.b);
        float max_r = dot_size * 0.5 * sqrt(ch_val) * contrast;
        float edge = dot_size * 0.05;
        float val = (dist < max_r) ? 1.0 : 0.0;
        if (edge > 0.5 && dist >= max_r - edge && dist < max_r + edge) {
            val = clamp(1.0 - (dist - (max_r - edge)) / (2.0 * edge), 0.0, 1.0);
        }
        if (c == 0) result.r = val;
        else if (c == 1) result.g = val;
        else result.b = val;
    }
    return result;
}

kernel void artistic_filter_kernel(
    texture2d<float, access::read>  srcTex [[texture(0)]],
    texture2d<float, access::write> dstTex [[texture(1)]],
    constant ArtisticParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    int x = int(gid.x);
    int y = int(gid.y);
    if (x >= params.width || y >= params.height) return;

    float4 src = srcTex.read(gid);
    float3 result;

    switch (params.filter_id) {
        case 0: { // oil_paint
            int r = int(params.p0 + params.p1 / 100.0 * 10.0);
            r = clamp(r, 1, 20);
            result = oil_paint(srcTex, x, y, params.width, params.height, r);
            break;
        }
        case 2: { // sketch
            float t = clamp(params.p0 / 100.0, 0.0, 1.0);
            float d = clamp(params.p1 / 100.0, 0.0, 1.0);
            result = sketch_filter(srcTex, x, y, params.width, params.height, t, d);
            break;
        }
        case 3: { // pixelate
            int bs = clamp(int(params.p0), 2, 100);
            int bx = (x / bs) * bs + bs / 2;
            int by = (y / bs) * bs + bs / 2;
            bx = clamp(bx, 0, params.width - 1);
            by = clamp(by, 0, params.height - 1);
            result = srcTex.read(uint2(bx, by)).rgb;
            break;
        }
        case 5: { // halftone
            float ds = clamp(params.p0, 2.0, 40.0);
            float contrast = clamp(params.p2 / 100.0, 0.0, 2.0);
            result = halftone_filter(srcTex, x, y, params.width, params.height,
                                     ds, params.p1, contrast);
            break;
        }
        default:
            result = src.rgb;
            break;
    }

    dstTex.write(float4(result, src.a), gid);
}
)";

extern "C" const char* gopost_artistic_compute_shader_source() {
    return kArtisticComputeMSL;
}

#else

/* Non-Apple: no Metal support */

extern "C" const char* gopost_artistic_compute_shader_source() {
    return "";
}

#endif
