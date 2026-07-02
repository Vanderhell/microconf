# Design

## Intentional Constraints

- fixed-memory C99 core
- caller-owned schema, context, data, and backend objects
- no heap allocation
- no automatic schema migration claims
- no hidden threading or locking

## Canonical Persistence

Each slot contains:

- magic
- storage format version
- user schema version
- schema fingerprint
- entry count
- canonical payload length
- generation sequence
- payload CRC32
- committed state marker

Payload encoding is deterministic by schema order:

- bool: one byte, `0` or `1`
- u16/u32/i32: documented little-endian
- float32: IEEE binary32 little-endian when enabled
- string: `u16` length plus bytes, no embedded NUL
- blob: exact field bytes

## Concurrency and Ownership

Separate contexts and separate backends are reentrant. Shared data, shared
context instances, and shared storage backends must be serialized by the caller.
