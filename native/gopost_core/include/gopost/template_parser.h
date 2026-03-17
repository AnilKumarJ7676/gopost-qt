#ifndef GOPOST_TEMPLATE_PARSER_H
#define GOPOST_TEMPLATE_PARSER_H

#include "gopost/types.h"
#include "gopost/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GopostEngine GopostEngine;
typedef struct GopostTemplate GopostTemplate;

#define GOPOST_MAGIC_BYTES "GOPT"
#define GOPOST_MAGIC_SIZE 4
#define GOPOST_FILE_HEADER_SIZE 16
#define GOPOST_ENCRYPTION_HEADER_SIZE 64

typedef enum {
    GOPOST_TEMPLATE_TYPE_VIDEO = 1,
    GOPOST_TEMPLATE_TYPE_IMAGE = 2,
} GopostTemplateType;

typedef enum {
    GOPOST_SECTION_METADATA = 0,
    GOPOST_SECTION_LAYERS   = 1,
    GOPOST_SECTION_ASSETS   = 2,
    GOPOST_SECTION_RENDER   = 3,
    GOPOST_SECTION_BLOB     = 4,
} GopostSectionType;

GopostError gopost_template_parse(
    GopostEngine* engine,
    const uint8_t* data, size_t data_len,
    const uint8_t* session_key, size_t key_len,
    GopostTemplate** out_template
);

/** Parse .gpt with RSA-OAEP wrapped session key (unwrap then decrypt + parse). */
GopostError gopost_template_parse_with_wrapped_key(
    GopostEngine* engine,
    const uint8_t* data, size_t data_len,
    const uint8_t* wrapped_session_key, size_t wrapped_key_len,
    const uint8_t* private_key_der, size_t private_key_der_len,
    GopostTemplate** out_template
);

GopostError gopost_template_get_metadata(
    GopostTemplate* tpl,
    const char** out_json
);

GopostError gopost_template_get_layer_count(
    GopostTemplate* tpl,
    uint32_t* out_count
);

GopostError gopost_template_unload(
    GopostEngine* engine,
    GopostTemplate* tpl
);

#ifdef __cplusplus
}
#endif

#endif
