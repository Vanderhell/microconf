/*
 * microconf - Type-safe configuration manager for embedded systems.
 *
 * C99 - Zero dependencies - Zero allocations - Flash-friendly - Portable
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microconf
 */

#ifndef MCONF_H
#define MCONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* -- Configuration ------------------------------------------------------- */

#ifndef MCONF_MAX_ENTRIES
#define MCONF_MAX_ENTRIES 32
#endif

#ifndef MCONF_MAX_KEY_LEN
#define MCONF_MAX_KEY_LEN 24
#endif

#ifndef MCONF_ENABLE_NAMES
#define MCONF_ENABLE_NAMES 1
#endif

/* -- Assert -------------------------------------------------------------- */

#ifdef MCONF_ASSERT
#define MCONF_CHECK_NULL(ptr) do { MCONF_ASSERT((ptr) != NULL); } while (0)
#else
#define MCONF_CHECK_NULL(ptr) do { if ((ptr) == NULL) return MCONF_ERR_NULL; } while (0)
#endif

/* -- Error codes --------------------------------------------------------- */

typedef enum {
    MCONF_OK              =  0,   /**< Success.                            */
    MCONF_ERR_NULL        = -1,   /**< NULL pointer argument.              */
    MCONF_ERR_NOT_FOUND   = -2,   /**< Key not found in schema.            */
    MCONF_ERR_TYPE        = -3,   /**< Type mismatch.                      */
    MCONF_ERR_SIZE        = -4,   /**< Buffer too small or size mismatch.  */
    MCONF_ERR_CRC         = -5,   /**< CRC validation failed.              */
    MCONF_ERR_MAGIC       = -6,   /**< Invalid magic / not initialised.    */
    MCONF_ERR_VERSION     = -7,   /**< Schema version mismatch.            */
    MCONF_ERR_IO          = -8,   /**< Platform read/write/erase failed.   */
    MCONF_ERR_INVALID     = -9,   /**< Invalid schema or configuration.    */
    MCONF_ERR_RANGE       = -10,  /**< Value out of allowed range.         */
} mconf_err_t;

/** Convert error code to human-readable string. */
const char *mconf_err_str(mconf_err_t err);

/* -- Field types --------------------------------------------------------- */

typedef enum {
    MCONF_TYPE_BOOL   = 0,    /**< bool (1 byte).                        */
    MCONF_TYPE_U8     = 1,    /**< uint8_t.                              */
    MCONF_TYPE_I8     = 2,    /**< int8_t.                               */
    MCONF_TYPE_U16    = 3,    /**< uint16_t.                             */
    MCONF_TYPE_I16    = 4,    /**< int16_t.                              */
    MCONF_TYPE_U32    = 5,    /**< uint32_t.                             */
    MCONF_TYPE_I32    = 6,    /**< int32_t.                              */
    MCONF_TYPE_FLOAT  = 7,    /**< float (IEEE 754, 4 bytes).            */
    MCONF_TYPE_STR    = 8,    /**< Fixed-size char array (NUL-terminated).*/
    MCONF_TYPE_BLOB   = 9,    /**< Raw byte array.                       */
} mconf_type_t;

/** Get human-readable name for a type. */
const char *mconf_type_name(mconf_type_t type);

/* -- Schema entry -------------------------------------------------------- */

/**
 * Describes one configuration field.
 *
 * The schema is a const array defined at compile time. It maps field names
 * to their location in a user-defined configuration struct.
 */
typedef struct {
#if MCONF_ENABLE_NAMES
    const char    *key;         /**< Field name (e.g., "wifi_ssid").       */
#endif
    uint16_t       offset;      /**< Byte offset in the config struct.     */
    uint16_t       size;        /**< Size in bytes.                        */
    mconf_type_t   type;        /**< Field type.                           */
    const void    *default_val; /**< Pointer to default value (or NULL).   */
} mconf_entry_t;

/* -- Helper macros for schema definition --------------------------------- */

/** Helper: define a schema entry with a default value. */
#if MCONF_ENABLE_NAMES
#define MCONF_ENTRY(struct_type, field, ftype, defptr) \
    { .key = #field,                                    \
      .offset = offsetof(struct_type, field),           \
      .size = sizeof(((struct_type *)0)->field),        \
      .type = (ftype),                                  \
      .default_val = (defptr) }
#else
#define MCONF_ENTRY(struct_type, field, ftype, defptr) \
    { .offset = offsetof(struct_type, field),           \
      .size = sizeof(((struct_type *)0)->field),        \
      .type = (ftype),                                  \
      .default_val = (defptr) }
#endif

/* -- Storage header ------------------------------------------------------ */

#define MCONF_MAGIC 0x434F4E46U   /* "CONF" in ASCII */

/**
 * Header prepended to the serialised config blob in flash.
 * Total: 16 bytes, naturally aligned.
 */
typedef struct {
    uint32_t magic;          /**< MCONF_MAGIC.                            */
    uint16_t version;        /**< Schema version (user-defined).          */
    uint16_t num_entries;    /**< Number of entries in schema.            */
    uint32_t data_size;      /**< Size of config data (excl. header).     */
    uint32_t crc32;          /**< CRC32 of config data bytes.             */
} mconf_header_t;

/* -- Platform callbacks -------------------------------------------------- */

/**
 * Read bytes from persistent storage.
 *
 * @param offset  Byte offset from storage base.
 * @param buf     Destination buffer.
 * @param len     Number of bytes to read.
 * @return 0 on success, negative on failure.
 */
typedef int (*mconf_read_fn)(uint32_t offset, void *buf, uint32_t len);

/**
 * Write bytes to persistent storage.
 *
 * @param offset  Byte offset from storage base.
 * @param buf     Source buffer.
 * @param len     Number of bytes to write.
 * @return 0 on success, negative on failure.
 */
typedef int (*mconf_write_fn)(uint32_t offset, const void *buf, uint32_t len);

/**
 * Erase storage region (optional, may be NULL).
 *
 * @param offset  Byte offset from storage base.
 * @param len     Number of bytes to erase.
 * @return 0 on success, negative on failure.
 */
typedef int (*mconf_erase_fn)(uint32_t offset, uint32_t len);

/** Platform I/O bundle. */
typedef struct {
    mconf_read_fn   read;
    mconf_write_fn  write;
    mconf_erase_fn  erase;     /**< May be NULL if not needed. */
} mconf_io_t;

/* -- Schema definition --------------------------------------------------- */

/**
 * Complete schema - describes the layout and defaults for a config struct.
 * This is const and may reside in ROM.
 */
typedef struct {
    const mconf_entry_t  *entries;       /**< Array of field descriptors.  */
    uint8_t               num_entries;   /**< Number of entries.           */
    uint16_t              version;       /**< Schema version number.       */
    uint16_t              data_size;     /**< sizeof(your config struct).  */
} mconf_schema_t;

/* -- Core API ------------------------------------------------------------ */

/**
 * Load defaults into a config struct.
 *
 * Zeroes the struct, then copies each entry's default_val into the
 * corresponding offset. Entries without a default (NULL) stay zero.
 *
 * @param schema  Schema definition.
 * @param data    Pointer to config struct (caller-allocated).
 * @return MCONF_OK on success.
 */
mconf_err_t mconf_load_defaults(const mconf_schema_t *schema, void *data);

/**
 * Load config from persistent storage.
 *
 * Reads header + data, validates magic/version/CRC, and copies into the
 * config struct. If validation fails, loads defaults instead and returns
 * the specific error code (MCONF_ERR_CRC, MCONF_ERR_MAGIC, etc.).
 *
 * @param schema  Schema definition.
 * @param data    Pointer to config struct (caller-allocated).
 * @param io      Platform I/O callbacks.
 * @return MCONF_OK if loaded from storage, error code if defaults were used.
 */
mconf_err_t mconf_load(const mconf_schema_t *schema, void *data,
                        const mconf_io_t *io);

/**
 * Save config to persistent storage.
 *
 * Computes CRC32, builds header, optionally erases, then writes header +
 * data as a contiguous blob.
 *
 * @param schema  Schema definition.
 * @param data    Pointer to config struct.
 * @param io      Platform I/O callbacks.
 * @return MCONF_OK on success.
 */
mconf_err_t mconf_save(const mconf_schema_t *schema, const void *data,
                        const mconf_io_t *io);

/**
 * Validate config data against schema (CRC, ranges, string termination).
 *
 * @param schema  Schema definition.
 * @param data    Pointer to config struct.
 * @return MCONF_OK if valid.
 */
mconf_err_t mconf_validate(const mconf_schema_t *schema, const void *data);

/* -- Field access API ---------------------------------------------------- */

/**
 * Find a schema entry by key name.
 * Returns the entry index (0-based) or -1 if not found.
 * Only available when MCONF_ENABLE_NAMES == 1.
 */
int mconf_find(const mconf_schema_t *schema, const char *key);

/**
 * Get a field value by entry index.
 *
 * @param schema  Schema definition.
 * @param data    Config struct.
 * @param index   Entry index (0 .. num_entries-1).
 * @param out     Destination buffer (must be >= entry.size).
 * @param out_size Size of destination buffer.
 * @return MCONF_OK on success.
 */
mconf_err_t mconf_get(const mconf_schema_t *schema, const void *data,
                       uint8_t index, void *out, uint16_t out_size);

/**
 * Set a field value by entry index.
 *
 * @param schema  Schema definition.
 * @param data    Config struct.
 * @param index   Entry index (0 .. num_entries-1).
 * @param value   Source buffer (must be >= entry.size).
 * @param val_size Size of source buffer.
 * @return MCONF_OK on success.
 */
mconf_err_t mconf_set(const mconf_schema_t *schema, void *data,
                       uint8_t index, const void *value, uint16_t val_size);

/**
 * Reset a single field to its default value.
 *
 * @param schema  Schema definition.
 * @param data    Config struct.
 * @param index   Entry index.
 * @return MCONF_OK on success.
 */
mconf_err_t mconf_reset_field(const mconf_schema_t *schema, void *data,
                               uint8_t index);

/* -- Convenience typed getters/setters ----------------------------------- */

mconf_err_t mconf_get_bool(const mconf_schema_t *s, const void *d, uint8_t i, bool *out);
mconf_err_t mconf_set_bool(const mconf_schema_t *s, void *d, uint8_t i, bool val);

mconf_err_t mconf_get_u8(const mconf_schema_t *s, const void *d, uint8_t i, uint8_t *out);
mconf_err_t mconf_set_u8(const mconf_schema_t *s, void *d, uint8_t i, uint8_t val);

mconf_err_t mconf_get_u16(const mconf_schema_t *s, const void *d, uint8_t i, uint16_t *out);
mconf_err_t mconf_set_u16(const mconf_schema_t *s, void *d, uint8_t i, uint16_t val);

mconf_err_t mconf_get_u32(const mconf_schema_t *s, const void *d, uint8_t i, uint32_t *out);
mconf_err_t mconf_set_u32(const mconf_schema_t *s, void *d, uint8_t i, uint32_t val);

mconf_err_t mconf_get_i32(const mconf_schema_t *s, const void *d, uint8_t i, int32_t *out);
mconf_err_t mconf_set_i32(const mconf_schema_t *s, void *d, uint8_t i, int32_t val);

mconf_err_t mconf_get_float(const mconf_schema_t *s, const void *d, uint8_t i, float *out);
mconf_err_t mconf_set_float(const mconf_schema_t *s, void *d, uint8_t i, float val);

mconf_err_t mconf_get_str(const mconf_schema_t *s, const void *d, uint8_t i,
                           char *out, uint16_t out_size);
mconf_err_t mconf_set_str(const mconf_schema_t *s, void *d, uint8_t i, const char *val);

/* -- CRC utility --------------------------------------------------------- */

/** Compute CRC32 (ISO 3309 / ITU-T V.42). */
uint32_t mconf_crc32(const void *data, uint32_t len);

/* -- Schema validation --------------------------------------------------- */

/**
 * Validate a schema for structural errors.
 *
 * Checks: non-NULL pointers, no overlapping fields, offsets within
 * data_size, type/size consistency.
 *
 * @param schema  Schema to validate.
 * @return MCONF_OK if valid.
 */
mconf_err_t mconf_schema_validate(const mconf_schema_t *schema);

#ifdef __cplusplus
}
#endif

#endif /* MCONF_H */
