#ifndef GOPOST_TYPES_H
#define GOPOST_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    GOPOST_PIXEL_FORMAT_RGBA8 = 0,
    GOPOST_PIXEL_FORMAT_BGRA8 = 1,
    GOPOST_PIXEL_FORMAT_NV12  = 2,
    GOPOST_PIXEL_FORMAT_YUV420P = 3,
} GopostPixelFormat;

typedef struct {
    uint32_t width;
    uint32_t height;
    GopostPixelFormat format;
    uint8_t* data;
    size_t data_size;
    size_t stride;
} GopostFrame;

typedef struct {
    float x;
    float y;
    float width;
    float height;
} GopostRect;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} GopostColor;

/* ── Opaque GPU resource handles ──────────────────────────────────────────── */

typedef uint64_t GopostTextureHandle;
typedef uint64_t GopostFramebufferHandle;
typedef uint64_t GopostShaderHandle;

#define GOPOST_NULL_HANDLE ((uint64_t)0)

/* ── Texture usage flags (bitwise OR) ─────────────────────────────────────── */

typedef enum {
    GOPOST_TEXTURE_USAGE_SAMPLED       = 0x01,
    GOPOST_TEXTURE_USAGE_RENDER_TARGET = 0x02,
    GOPOST_TEXTURE_USAGE_STORAGE       = 0x04,
    GOPOST_TEXTURE_USAGE_TRANSFER_SRC  = 0x08,
    GOPOST_TEXTURE_USAGE_TRANSFER_DST  = 0x10,
} GopostTextureUsage;

/* ── Shader enums ─────────────────────────────────────────────────────────── */

typedef enum {
    GOPOST_SHADER_STAGE_VERTEX   = 0,
    GOPOST_SHADER_STAGE_FRAGMENT = 1,
    GOPOST_SHADER_STAGE_COMPUTE  = 2,
} GopostShaderStage;

typedef enum {
    GOPOST_SHADER_FORMAT_SOURCE = 0,
    GOPOST_SHADER_FORMAT_SPIRV  = 1,
    GOPOST_SHADER_FORMAT_DXBC   = 2,
} GopostShaderFormat;

/* ── Uniform types ────────────────────────────────────────────────────────── */

typedef enum {
    GOPOST_UNIFORM_INT   = 0,
    GOPOST_UNIFORM_FLOAT = 1,
    GOPOST_UNIFORM_VEC2  = 2,
    GOPOST_UNIFORM_VEC3  = 3,
    GOPOST_UNIFORM_VEC4  = 4,
    GOPOST_UNIFORM_MAT4  = 5,
} GopostUniformType;

/* ── GPU descriptors ──────────────────────────────────────────────────────── */

typedef struct {
    uint32_t width;
    uint32_t height;
    GopostPixelFormat format;
    uint32_t usage;   /* bitwise OR of GopostTextureUsage */
} GopostTextureDesc;

typedef struct {
    GopostShaderStage  stage;
    GopostShaderFormat format;
    const void*        data;
    size_t             data_size;
    const char*        entry_point;  /* NULL = "main" */
} GopostShaderDesc;

typedef struct {
    GopostUniformType type;
    union {
        int32_t i;
        float   f;
        float   vec2[2];
        float   vec3[3];
        float   vec4[4];
        float   mat4[16];
    } value;
} GopostUniformValue;

#endif
