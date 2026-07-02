# Compile-Fail Cases

These files are intentionally invalid and are meant to be compiled as negative
gates in CI rather than linked into the library.

- `conflicting_config.c`: conflicting config override against installed header
- `invalid_string_default.c`: string default exceeds field capacity
- `invalid_schema_count.c`: schema count exceeds authoritative max
