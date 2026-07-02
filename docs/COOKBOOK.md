# Cookbook

## Initialize a Context

1. Define a plain caller-owned config struct.
2. Define `mconf_entry_t` entries with explicit default macros.
3. Define `mconf_schema_t`.
4. Call `mconf_init`.
5. Call `mconf_load_defaults` or `mconf_load`.

## Default Macros

- `MCONF_ENTRY_ZERO`
- `MCONF_ENTRY_SCALAR`
- `MCONF_ENTRY_STRING`
- `MCONF_ENTRY_BLOB`

## Common Operations

- load defaults into a new buffer
- reset one field to its schema default
- get/set typed scalars
- use `mconf_set_string` with explicit length and capacity reporting
- use blob APIs for embedded NUL data
- save and load through a two-slot backend

## Multi-Backend Use

Separate `mconf_t` contexts may target separate backends at the same time. A
single context must not be re-entered from its own callbacks.
