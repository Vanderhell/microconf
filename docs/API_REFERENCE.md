# API Reference

## Core Types

- `mconf_err_t`: fixed-width signed 32-bit error code type
- `mconf_type_t`: fixed-width unsigned 8-bit field type identifier
- `mconf_t`: validated caller-owned runtime context
- `mconf_schema_t`: schema metadata and entry table
- `mconf_io_t`: callback bundle for two-slot persistence

## Initialization

`mconf_init(&ctx, sizeof(ctx), &schema, &data, sizeof(data))`

Initializes a caller-owned context only after schema validation succeeds. On
failure the caller data buffer is left unchanged.

## Defaults

- `mconf_load_defaults`
- `mconf_reset_field`

Default rules:

- `default_value == NULL` requires `default_size == 0`
- scalar defaults must match field size exactly
- string defaults use byte count excluding NUL
- blob defaults must match field size exactly

## Access

- `mconf_get` / `mconf_set` use exact field size
- `mconf_get_bool`, `mconf_set_u16`, `mconf_set_float`, and similar typed APIs
- `mconf_get_string` / `mconf_set_string`
- `mconf_get_blob` / `mconf_set_blob`
- `mconf_find` returns `MCONF_ERR_UNSUPPORTED` when names are disabled

## Persistence

- `mconf_save`
- `mconf_load`

Storage is canonical and field-aware. The library does not write raw C header
structs or raw config object representation to persistent storage.
