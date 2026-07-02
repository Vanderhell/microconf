/*
 * microconf - fixed-memory configuration storage for embedded systems
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MCONF_H
#define MCONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mconf_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef int32_t mconf_err_t;
typedef uint8_t mconf_type_t;

#define MCONF_OK                         ((mconf_err_t)0)
#define MCONF_ERR_NULL                   ((mconf_err_t)-1)
#define MCONF_ERR_NOT_FOUND              ((mconf_err_t)-2)
#define MCONF_ERR_TYPE                   ((mconf_err_t)-3)
#define MCONF_ERR_SIZE                   ((mconf_err_t)-4)
#define MCONF_ERR_CRC                    ((mconf_err_t)-5)
#define MCONF_ERR_MAGIC                  ((mconf_err_t)-6)
#define MCONF_ERR_VERSION                ((mconf_err_t)-7)
#define MCONF_ERR_IO                     ((mconf_err_t)-8)
#define MCONF_ERR_INVALID                ((mconf_err_t)-9)
#define MCONF_ERR_RANGE                  ((mconf_err_t)-10)
#define MCONF_ERR_UNSUPPORTED            ((mconf_err_t)-11)
#define MCONF_ERR_UNINITIALIZED          ((mconf_err_t)-12)
#define MCONF_ERR_CONFLICT               ((mconf_err_t)-13)

#define MCONF_TYPE_BOOL                  ((mconf_type_t)1u)
#define MCONF_TYPE_U8                    ((mconf_type_t)2u)
#define MCONF_TYPE_I8                    ((mconf_type_t)3u)
#define MCONF_TYPE_U16                   ((mconf_type_t)4u)
#define MCONF_TYPE_I16                   ((mconf_type_t)5u)
#define MCONF_TYPE_U32                   ((mconf_type_t)6u)
#define MCONF_TYPE_I32                   ((mconf_type_t)7u)
#define MCONF_TYPE_FLOAT32               ((mconf_type_t)8u)
#define MCONF_TYPE_STRING                ((mconf_type_t)9u)
#define MCONF_TYPE_BLOB                  ((mconf_type_t)10u)

#if defined(MCONF_MAX_ENTRIES) && (MCONF_MAX_ENTRIES != MCONF_CONFIG_MAX_ENTRIES)
#error "Consumer override for MCONF_MAX_ENTRIES conflicts with mconf_config.h"
#endif
#if defined(MCONF_MAX_KEY_LEN) && (MCONF_MAX_KEY_LEN != MCONF_CONFIG_MAX_KEY_LEN)
#error "Consumer override for MCONF_MAX_KEY_LEN conflicts with mconf_config.h"
#endif
#if defined(MCONF_ENABLE_NAMES) && (MCONF_ENABLE_NAMES != MCONF_CONFIG_ENABLE_NAMES)
#error "Consumer override for MCONF_ENABLE_NAMES conflicts with mconf_config.h"
#endif
#if defined(MCONF_ENABLE_FLOAT) && (MCONF_ENABLE_FLOAT != MCONF_CONFIG_ENABLE_FLOAT)
#error "Consumer override for MCONF_ENABLE_FLOAT conflicts with mconf_config.h"
#endif

#define MCONF_MAX_ENTRIES MCONF_CONFIG_MAX_ENTRIES
#define MCONF_MAX_KEY_LEN MCONF_CONFIG_MAX_KEY_LEN
#define MCONF_ENABLE_NAMES MCONF_CONFIG_ENABLE_NAMES
#define MCONF_ENABLE_FLOAT MCONF_CONFIG_ENABLE_FLOAT

#if defined(MCONF_ASSERT)
#define MCONF_ASSERT_RUNTIME(expr) do { MCONF_ASSERT(expr); } while (0)
#else
#define MCONF_ASSERT_RUNTIME(expr) do { (void)(expr); } while (0)
#endif

#define MCONF_SCHEMA_VERSION_CURRENT     ((uint32_t)1u)
#define MCONF_STORAGE_FORMAT_VERSION     ((uint16_t)1u)

typedef struct {
    const char *key;
    size_t offset;
    size_t size;
    mconf_type_t type;
    const void *default_value;
    size_t default_size;
} mconf_entry_t;

typedef struct {
    const mconf_entry_t *entries;
    size_t entry_count;
    uint32_t schema_version;
    size_t data_size;
} mconf_schema_t;

typedef struct {
    const mconf_schema_t *schema;
    void *data;
    size_t data_size;
    uint32_t schema_fingerprint;
    bool initialized;
} mconf_t;

typedef int (*mconf_read_fn)(void *callback_ctx, size_t offset, void *buffer, size_t size);
typedef int (*mconf_write_fn)(void *callback_ctx, size_t offset, const void *buffer, size_t size);
typedef int (*mconf_erase_fn)(void *callback_ctx, size_t offset, size_t size);

typedef struct {
    void *callback_ctx;
    size_t storage_size;
    size_t slot_size;
    mconf_read_fn read;
    mconf_write_fn write;
    mconf_erase_fn erase;
} mconf_io_t;

#define MCONF_KEY_LITERAL(field_name) ((MCONF_ENABLE_NAMES != 0) ? #field_name : (const char *)0)

#define MCONF_ENTRY_ZERO(struct_type, field_name, field_type) \
    { MCONF_KEY_LITERAL(field_name), offsetof(struct_type, field_name), \
      sizeof(((struct_type *)0)->field_name), (field_type), (const void *)0, (size_t)0u }

#define MCONF_ENTRY_SCALAR(struct_type, field_name, field_type, default_ptr) \
    { MCONF_KEY_LITERAL(field_name), offsetof(struct_type, field_name), \
      sizeof(((struct_type *)0)->field_name), (field_type), (default_ptr), sizeof(*(default_ptr)) }

#define MCONF_ENTRY_STRING(struct_type, field_name, literal_value) \
    { MCONF_KEY_LITERAL(field_name), offsetof(struct_type, field_name), \
      sizeof(((struct_type *)0)->field_name), MCONF_TYPE_STRING, \
      (literal_value), sizeof(literal_value) - (size_t)1u }

#define MCONF_ENTRY_BLOB(struct_type, field_name, default_ptr, default_len) \
    { MCONF_KEY_LITERAL(field_name), offsetof(struct_type, field_name), \
      sizeof(((struct_type *)0)->field_name), MCONF_TYPE_BLOB, (default_ptr), (default_len) }

const char *mconf_err_str(mconf_err_t err);
const char *mconf_type_name(mconf_type_t type);
uint32_t mconf_crc32(const void *data, size_t size);

mconf_err_t mconf_schema_validate(const mconf_schema_t *schema, uint32_t *fingerprint_out);
mconf_err_t mconf_validate_data(const mconf_t *ctx, const void *data);
mconf_err_t mconf_init(mconf_t *ctx, size_t ctx_size, const mconf_schema_t *schema, void *data, size_t data_size);
mconf_err_t mconf_load_defaults(mconf_t *ctx);
mconf_err_t mconf_reset_field(mconf_t *ctx, size_t index);

mconf_err_t mconf_find(const mconf_t *ctx, const char *key, size_t *index_out);

mconf_err_t mconf_get(const mconf_t *ctx, size_t index, void *out, size_t out_size);
mconf_err_t mconf_set(mconf_t *ctx, size_t index, const void *value, size_t value_size);

mconf_err_t mconf_get_bool(const mconf_t *ctx, size_t index, bool *out);
mconf_err_t mconf_set_bool(mconf_t *ctx, size_t index, bool value);
mconf_err_t mconf_get_u8(const mconf_t *ctx, size_t index, uint8_t *out);
mconf_err_t mconf_set_u8(mconf_t *ctx, size_t index, uint8_t value);
mconf_err_t mconf_get_u16(const mconf_t *ctx, size_t index, uint16_t *out);
mconf_err_t mconf_set_u16(mconf_t *ctx, size_t index, uint16_t value);
mconf_err_t mconf_get_u32(const mconf_t *ctx, size_t index, uint32_t *out);
mconf_err_t mconf_set_u32(mconf_t *ctx, size_t index, uint32_t value);
mconf_err_t mconf_get_i32(const mconf_t *ctx, size_t index, int32_t *out);
mconf_err_t mconf_set_i32(mconf_t *ctx, size_t index, int32_t value);
mconf_err_t mconf_get_float(const mconf_t *ctx, size_t index, float *out);
mconf_err_t mconf_set_float(mconf_t *ctx, size_t index, float value);
mconf_err_t mconf_get_blob(const mconf_t *ctx, size_t index, void *out, size_t out_size);
mconf_err_t mconf_set_blob(mconf_t *ctx, size_t index, const void *value, size_t value_size);
mconf_err_t mconf_get_string(const mconf_t *ctx, size_t index, char *out, size_t out_size, size_t *required_size_out);
mconf_err_t mconf_set_string(mconf_t *ctx, size_t index, const char *value, size_t value_length, size_t *required_capacity_out);

mconf_err_t mconf_load(mconf_t *ctx, const mconf_io_t *io);
mconf_err_t mconf_save(mconf_t *ctx, const mconf_io_t *io);

#ifdef __cplusplus
}
#endif

#endif
