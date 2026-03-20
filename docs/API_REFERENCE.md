# API Reference

> **Header:** `#include "mconf.h"`
>
> **Version:** 1.0.0

---

## Error codes

```c
typedef enum {
    MCONF_OK              =  0,
    MCONF_ERR_NULL        = -1,
    MCONF_ERR_NOT_FOUND   = -2,
    MCONF_ERR_TYPE        = -3,
    MCONF_ERR_SIZE        = -4,
    MCONF_ERR_CRC         = -5,
    MCONF_ERR_MAGIC       = -6,
    MCONF_ERR_VERSION     = -7,
    MCONF_ERR_IO          = -8,
    MCONF_ERR_INVALID     = -9,
    MCONF_ERR_RANGE       = -10,
} mconf_err_t;
```

`mconf_err_str(err)` converts any code to a human-readable string.

---

## Field types

| Type | C type | Size | Notes |
|------|--------|------|-------|
| `MCONF_TYPE_BOOL` | `bool` | 1 | |
| `MCONF_TYPE_U8` | `uint8_t` | 1 | |
| `MCONF_TYPE_I8` | `int8_t` | 1 | |
| `MCONF_TYPE_U16` | `uint16_t` | 2 | |
| `MCONF_TYPE_I16` | `int16_t` | 2 | |
| `MCONF_TYPE_U32` | `uint32_t` | 4 | |
| `MCONF_TYPE_I32` | `int32_t` | 4 | |
| `MCONF_TYPE_FLOAT` | `float` | 4 | IEEE 754 |
| `MCONF_TYPE_STR` | `char[]` | variable | Fixed-size, NUL-terminated |
| `MCONF_TYPE_BLOB` | `uint8_t[]` | variable | Raw bytes |

`mconf_type_name(type)` returns the type as a string.

---

## Schema definition

### mconf_entry_t

```c
typedef struct {
    const char *key;         /* field name (if MCONF_ENABLE_NAMES) */
    uint16_t    offset;      /* byte offset in config struct */
    uint16_t    size;        /* field size in bytes */
    mconf_type_t type;       /* field type */
    const void  *default_val; /* pointer to default (or NULL -> zero) */
} mconf_entry_t;
```

### MCONF_ENTRY macro

```c
MCONF_ENTRY(struct_type, field_name, type, default_ptr)
```

Auto-computes offset and size using `offsetof()` and `sizeof()`. The key
name is auto-generated from the field name. `default_ptr` may be NULL.

### mconf_schema_t

```c
typedef struct {
    const mconf_entry_t *entries;
    uint8_t              num_entries;
    uint16_t             version;
    uint16_t             data_size;   /* sizeof(your config struct) */
} mconf_schema_t;
```

---

## Storage format

```
+----------------------------------+  offset 0
|  mconf_header_t (16 bytes)       |
|    magic:    0x434F4E46 ("CONF") |
|    version:  schema version      |
|    num_entries: entry count       |
|    data_size: sizeof(config)     |
|    crc32:    CRC of data bytes   |
|----------------------------------|  offset 16
|  Raw config struct bytes         |
|    (data_size bytes)             |
`----------------------------------+
```

---

## Platform callbacks

### mconf_io_t

```c
typedef struct {
    mconf_read_fn   read;    /* required */
    mconf_write_fn  write;   /* required */
    mconf_erase_fn  erase;   /* optional (NULL if not needed) */
} mconf_io_t;
```

Each callback takes `(uint32_t offset, void/const void *buf, uint32_t len)`
and returns 0 on success, negative on failure.

---

## Core functions

### mconf_load_defaults

```c
mconf_err_t mconf_load_defaults(const mconf_schema_t *schema, void *data);
```

Zeroes the config struct, then applies each entry's default. Entries
without a default stay zero.

### mconf_load

```c
mconf_err_t mconf_load(const mconf_schema_t *schema, void *data,
                        const mconf_io_t *io);
```

Reads header + data from storage. Validates magic, version, data size,
and CRC32. If any check fails, loads defaults and returns the specific
error code. Returns `MCONF_OK` if data was loaded successfully from
storage.

### mconf_save

```c
mconf_err_t mconf_save(const mconf_schema_t *schema, const void *data,
                        const mconf_io_t *io);
```

Computes CRC32, builds header, optionally calls erase, then writes
header + data.

### mconf_validate

```c
mconf_err_t mconf_validate(const mconf_schema_t *schema, const void *data);
```

Checks that string fields are NUL-terminated. Returns `MCONF_ERR_INVALID`
if any string lacks a NUL byte.

---

## Field access functions

### mconf_find

```c
int mconf_find(const mconf_schema_t *schema, const char *key);
```

Returns entry index by key name, or -1 if not found. Only available when
`MCONF_ENABLE_NAMES == 1`.

### mconf_get / mconf_set

```c
mconf_err_t mconf_get(schema, data, index, out, out_size);
mconf_err_t mconf_set(schema, data, index, value, val_size);
```

Generic field access by index. Checks bounds and buffer size. For strings,
`mconf_set` clears the field and ensures NUL termination.

### mconf_reset_field

```c
mconf_err_t mconf_reset_field(schema, data, index);
```

Resets one field to its default (or zero if no default).

---

## Typed getters/setters

Each checks type against the schema and returns `MCONF_ERR_TYPE` on
mismatch.

```c
mconf_get_bool(schema, data, index, &out)
mconf_set_bool(schema, data, index, value)

mconf_get_u8 / set_u8
mconf_get_u16 / set_u16
mconf_get_u32 / set_u32
mconf_get_i32 / set_i32
mconf_get_float / set_float

mconf_get_str(schema, data, index, buf, buf_size)
mconf_set_str(schema, data, index, "string")
```

---

## Utility functions

### mconf_crc32

```c
uint32_t mconf_crc32(const void *data, uint32_t len);
```

ISO 3309 CRC32. Bitwise implementation (no lookup table - saves 1 KB ROM).

### mconf_schema_validate

```c
mconf_err_t mconf_schema_validate(const mconf_schema_t *schema);
```

Checks: non-NULL pointers, fields within data_size, type/size consistency,
no overlapping fields, string fields >= 2 bytes.

---

## Thread safety

microconf is not thread-safe. Protect shared config access with your
platform's mutex. The schema is `const` and can be shared freely.
