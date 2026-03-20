/*
 * microconf - Implementation.
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microconf
 */

#include "mconf.h"
#include <string.h>

/* -- Error strings ------------------------------------------------------- */

const char *mconf_err_str(mconf_err_t err)
{
    switch (err) {
    case MCONF_OK:            return "ok";
    case MCONF_ERR_NULL:      return "null pointer";
    case MCONF_ERR_NOT_FOUND: return "key not found";
    case MCONF_ERR_TYPE:      return "type mismatch";
    case MCONF_ERR_SIZE:      return "size mismatch";
    case MCONF_ERR_CRC:       return "crc failed";
    case MCONF_ERR_MAGIC:     return "invalid magic";
    case MCONF_ERR_VERSION:   return "version mismatch";
    case MCONF_ERR_IO:        return "io error";
    case MCONF_ERR_INVALID:   return "invalid schema";
    case MCONF_ERR_RANGE:     return "out of range";
    default:                  return "unknown error";
    }
}

const char *mconf_type_name(mconf_type_t type)
{
    switch (type) {
    case MCONF_TYPE_BOOL:  return "bool";
    case MCONF_TYPE_U8:    return "u8";
    case MCONF_TYPE_I8:    return "i8";
    case MCONF_TYPE_U16:   return "u16";
    case MCONF_TYPE_I16:   return "i16";
    case MCONF_TYPE_U32:   return "u32";
    case MCONF_TYPE_I32:   return "i32";
    case MCONF_TYPE_FLOAT: return "float";
    case MCONF_TYPE_STR:   return "str";
    case MCONF_TYPE_BLOB:  return "blob";
    default:               return "?";
    }
}

/* -- CRC32 (ISO 3309) --------------------------------------------------- */

/*
 * Bitwise CRC32 - no lookup table, saves 1 KB of ROM.
 * Slower than table-based, but config operations are infrequent.
 */
uint32_t mconf_crc32(const void *data, uint32_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFU;

    for (uint32_t i = 0; i < len; i++) {
        crc ^= p[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 1U) {
                crc = (crc >> 1) ^ 0xEDB88320U;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFFU;
}

/* -- Internal helpers ---------------------------------------------------- */

/** Get pointer to field data within the config struct. */
static inline void *field_ptr(void *data, const mconf_entry_t *e)
{
    return (uint8_t *)data + e->offset;
}

static inline const void *field_ptr_const(const void *data, const mconf_entry_t *e)
{
    return (const uint8_t *)data + e->offset;
}

/** Check that index is in range. */
static inline mconf_err_t check_index(const mconf_schema_t *schema, uint8_t index)
{
    if (index >= schema->num_entries) return MCONF_ERR_NOT_FOUND;
    return MCONF_OK;
}

/** Expected size for a scalar type. Returns 0 for variable-size types. */
static uint16_t type_expected_size(mconf_type_t type)
{
    switch (type) {
    case MCONF_TYPE_BOOL:  return 1;
    case MCONF_TYPE_U8:    return 1;
    case MCONF_TYPE_I8:    return 1;
    case MCONF_TYPE_U16:   return 2;
    case MCONF_TYPE_I16:   return 2;
    case MCONF_TYPE_U32:   return 4;
    case MCONF_TYPE_I32:   return 4;
    case MCONF_TYPE_FLOAT: return 4;
    case MCONF_TYPE_STR:   return 0;  /* variable */
    case MCONF_TYPE_BLOB:  return 0;  /* variable */
    default:               return 0;
    }
}

/* -- Core API ------------------------------------------------------------ */

mconf_err_t mconf_load_defaults(const mconf_schema_t *schema, void *data)
{
    MCONF_CHECK_NULL(schema);
    MCONF_CHECK_NULL(data);
    MCONF_CHECK_NULL(schema->entries);

    /* Zero everything first */
    memset(data, 0, schema->data_size);

    /* Apply defaults */
    for (uint8_t i = 0; i < schema->num_entries; i++) {
        const mconf_entry_t *e = &schema->entries[i];
        if (e->default_val != NULL) {
            memcpy(field_ptr(data, e), e->default_val, e->size);
        }
    }

    return MCONF_OK;
}

mconf_err_t mconf_load(const mconf_schema_t *schema, void *data,
                        const mconf_io_t *io)
{
    MCONF_CHECK_NULL(schema);
    MCONF_CHECK_NULL(data);
    MCONF_CHECK_NULL(io);
    MCONF_CHECK_NULL(io->read);

    /* Read header */
    mconf_header_t header;
    if (io->read(0, &header, sizeof(header)) != 0) {
        mconf_load_defaults(schema, data);
        return MCONF_ERR_IO;
    }

    /* Validate magic */
    if (header.magic != MCONF_MAGIC) {
        mconf_load_defaults(schema, data);
        return MCONF_ERR_MAGIC;
    }

    /* Validate version */
    if (header.version != schema->version) {
        mconf_load_defaults(schema, data);
        return MCONF_ERR_VERSION;
    }

    /* Validate data size */
    if (header.data_size != schema->data_size) {
        mconf_load_defaults(schema, data);
        return MCONF_ERR_SIZE;
    }

    /* Read data */
    if (io->read(sizeof(header), data, schema->data_size) != 0) {
        mconf_load_defaults(schema, data);
        return MCONF_ERR_IO;
    }

    /* Validate CRC */
    uint32_t crc = mconf_crc32(data, schema->data_size);
    if (crc != header.crc32) {
        mconf_load_defaults(schema, data);
        return MCONF_ERR_CRC;
    }

    return MCONF_OK;
}

mconf_err_t mconf_save(const mconf_schema_t *schema, const void *data,
                        const mconf_io_t *io)
{
    MCONF_CHECK_NULL(schema);
    MCONF_CHECK_NULL(data);
    MCONF_CHECK_NULL(io);
    MCONF_CHECK_NULL(io->write);

    /* Build header */
    mconf_header_t header = {
        .magic       = MCONF_MAGIC,
        .version     = schema->version,
        .num_entries = schema->num_entries,
        .data_size   = schema->data_size,
        .crc32       = mconf_crc32(data, schema->data_size),
    };

    /* Erase if callback provided */
    if (io->erase != NULL) {
        uint32_t total = sizeof(header) + schema->data_size;
        if (io->erase(0, total) != 0) {
            return MCONF_ERR_IO;
        }
    }

    /* Write header */
    if (io->write(0, &header, sizeof(header)) != 0) {
        return MCONF_ERR_IO;
    }

    /* Write data */
    if (io->write(sizeof(header), data, schema->data_size) != 0) {
        return MCONF_ERR_IO;
    }

    return MCONF_OK;
}

mconf_err_t mconf_validate(const mconf_schema_t *schema, const void *data)
{
    MCONF_CHECK_NULL(schema);
    MCONF_CHECK_NULL(data);

    for (uint8_t i = 0; i < schema->num_entries; i++) {
        const mconf_entry_t *e = &schema->entries[i];

        /* Check string fields are NUL-terminated */
        if (e->type == MCONF_TYPE_STR) {
            const char *str = (const char *)field_ptr_const(data, e);
            bool terminated = false;
            for (uint16_t j = 0; j < e->size; j++) {
                if (str[j] == '\0') {
                    terminated = true;
                    break;
                }
            }
            if (!terminated) {
                return MCONF_ERR_INVALID;
            }
        }
    }

    return MCONF_OK;
}

/* -- Field access API ---------------------------------------------------- */

int mconf_find(const mconf_schema_t *schema, const char *key)
{
#if MCONF_ENABLE_NAMES
    if (schema == NULL || key == NULL || schema->entries == NULL) {
        return -1;
    }

    for (uint8_t i = 0; i < schema->num_entries; i++) {
        if (schema->entries[i].key != NULL &&
            strcmp(schema->entries[i].key, key) == 0) {
            return (int)i;
        }
    }
#else
    (void)schema;
    (void)key;
#endif
    return -1;
}

mconf_err_t mconf_get(const mconf_schema_t *schema, const void *data,
                       uint8_t index, void *out, uint16_t out_size)
{
    MCONF_CHECK_NULL(schema);
    MCONF_CHECK_NULL(data);
    MCONF_CHECK_NULL(out);

    mconf_err_t err = check_index(schema, index);
    if (err != MCONF_OK) return err;

    const mconf_entry_t *e = &schema->entries[index];

    if (out_size < e->size) {
        return MCONF_ERR_SIZE;
    }

    memcpy(out, field_ptr_const(data, e), e->size);
    return MCONF_OK;
}

mconf_err_t mconf_set(const mconf_schema_t *schema, void *data,
                       uint8_t index, const void *value, uint16_t val_size)
{
    MCONF_CHECK_NULL(schema);
    MCONF_CHECK_NULL(data);
    MCONF_CHECK_NULL(value);

    mconf_err_t err = check_index(schema, index);
    if (err != MCONF_OK) return err;

    const mconf_entry_t *e = &schema->entries[index];

    if (val_size > e->size) {
        return MCONF_ERR_SIZE;
    }

    /* For strings: clear field first, then copy (ensures NUL termination) */
    if (e->type == MCONF_TYPE_STR) {
        memset(field_ptr(data, e), 0, e->size);
        /* Copy at most size-1 bytes to guarantee NUL terminator */
        uint16_t copy_len = (val_size < e->size) ? val_size : (e->size - 1);
        memcpy(field_ptr(data, e), value, copy_len);
    } else {
        memcpy(field_ptr(data, e), value, val_size);
    }

    return MCONF_OK;
}

mconf_err_t mconf_reset_field(const mconf_schema_t *schema, void *data,
                               uint8_t index)
{
    MCONF_CHECK_NULL(schema);
    MCONF_CHECK_NULL(data);

    mconf_err_t err = check_index(schema, index);
    if (err != MCONF_OK) return err;

    const mconf_entry_t *e = &schema->entries[index];

    if (e->default_val != NULL) {
        memcpy(field_ptr(data, e), e->default_val, e->size);
    } else {
        memset(field_ptr(data, e), 0, e->size);
    }

    return MCONF_OK;
}

/* -- Convenience typed getters/setters ----------------------------------- */

#define TYPED_GETTER(suffix, ctype, mtype)                                 \
mconf_err_t mconf_get_##suffix(const mconf_schema_t *s, const void *d,     \
                                uint8_t i, ctype *out) {                   \
    MCONF_CHECK_NULL(s); MCONF_CHECK_NULL(d); MCONF_CHECK_NULL(out);       \
    mconf_err_t e = check_index(s, i);                                     \
    if (e != MCONF_OK) return e;                                           \
    if (s->entries[i].type != (mtype)) return MCONF_ERR_TYPE;              \
    memcpy(out, field_ptr_const(d, &s->entries[i]), sizeof(ctype));         \
    return MCONF_OK;                                                       \
}

#define TYPED_SETTER(suffix, ctype, mtype)                                 \
mconf_err_t mconf_set_##suffix(const mconf_schema_t *s, void *d,           \
                                uint8_t i, ctype val) {                    \
    MCONF_CHECK_NULL(s); MCONF_CHECK_NULL(d);                              \
    mconf_err_t e = check_index(s, i);                                     \
    if (e != MCONF_OK) return e;                                           \
    if (s->entries[i].type != (mtype)) return MCONF_ERR_TYPE;              \
    memcpy(field_ptr(d, &s->entries[i]), &val, sizeof(ctype));             \
    return MCONF_OK;                                                       \
}

TYPED_GETTER(bool,  bool,     MCONF_TYPE_BOOL)
TYPED_SETTER(bool,  bool,     MCONF_TYPE_BOOL)
TYPED_GETTER(u8,    uint8_t,  MCONF_TYPE_U8)
TYPED_SETTER(u8,    uint8_t,  MCONF_TYPE_U8)
TYPED_GETTER(u16,   uint16_t, MCONF_TYPE_U16)
TYPED_SETTER(u16,   uint16_t, MCONF_TYPE_U16)
TYPED_GETTER(u32,   uint32_t, MCONF_TYPE_U32)
TYPED_SETTER(u32,   uint32_t, MCONF_TYPE_U32)
TYPED_GETTER(i32,   int32_t,  MCONF_TYPE_I32)
TYPED_SETTER(i32,   int32_t,  MCONF_TYPE_I32)
TYPED_GETTER(float, float,    MCONF_TYPE_FLOAT)
TYPED_SETTER(float, float,    MCONF_TYPE_FLOAT)

mconf_err_t mconf_get_str(const mconf_schema_t *s, const void *d, uint8_t i,
                           char *out, uint16_t out_size)
{
    MCONF_CHECK_NULL(s); MCONF_CHECK_NULL(d); MCONF_CHECK_NULL(out);
    mconf_err_t err = check_index(s, i);
    if (err != MCONF_OK) return err;

    const mconf_entry_t *e = &s->entries[i];
    if (e->type != MCONF_TYPE_STR) return MCONF_ERR_TYPE;
    if (out_size < e->size) return MCONF_ERR_SIZE;

    memcpy(out, field_ptr_const(d, e), e->size);
    /* Ensure NUL termination */
    out[e->size - 1] = '\0';
    return MCONF_OK;
}

mconf_err_t mconf_set_str(const mconf_schema_t *s, void *d, uint8_t i,
                           const char *val)
{
    MCONF_CHECK_NULL(s); MCONF_CHECK_NULL(d); MCONF_CHECK_NULL(val);
    mconf_err_t err = check_index(s, i);
    if (err != MCONF_OK) return err;

    const mconf_entry_t *e = &s->entries[i];
    if (e->type != MCONF_TYPE_STR) return MCONF_ERR_TYPE;

    /* Clear and copy with NUL guarantee */
    memset(field_ptr(d, e), 0, e->size);
    uint16_t len = 0;
    while (val[len] != '\0' && len < e->size - 1) len++;
    memcpy(field_ptr(d, e), val, len);

    return MCONF_OK;
}

/* -- Schema validation --------------------------------------------------- */

mconf_err_t mconf_schema_validate(const mconf_schema_t *schema)
{
    MCONF_CHECK_NULL(schema);
    MCONF_CHECK_NULL(schema->entries);

    if (schema->num_entries == 0 || schema->data_size == 0) {
        return MCONF_ERR_INVALID;
    }

    for (uint8_t i = 0; i < schema->num_entries; i++) {
        const mconf_entry_t *e = &schema->entries[i];

        /* Check field fits within data_size */
        if ((uint32_t)e->offset + e->size > schema->data_size) {
            return MCONF_ERR_INVALID;
        }

        /* Check type/size consistency for scalar types */
        uint16_t expected = type_expected_size(e->type);
        if (expected > 0 && e->size != expected) {
            return MCONF_ERR_INVALID;
        }

        /* Strings must have at least 2 bytes (1 char + NUL) */
        if (e->type == MCONF_TYPE_STR && e->size < 2) {
            return MCONF_ERR_INVALID;
        }

        /* Check for overlapping fields */
        for (uint8_t j = i + 1; j < schema->num_entries; j++) {
            const mconf_entry_t *f = &schema->entries[j];
            uint16_t e_end = e->offset + e->size;
            uint16_t f_end = f->offset + f->size;

            if (e->offset < f_end && f->offset < e_end) {
                return MCONF_ERR_INVALID;  /* overlap */
            }
        }
    }

    return MCONF_OK;
}
