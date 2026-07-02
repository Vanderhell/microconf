# Issues And Troubleshooting

Common failure classes:

- invalid defaults or default sizes
- schema overlap or duplicate keys
- conflicting installed config header values
- caller data size mismatch at `mconf_init`
- string too long for fixed field capacity
- partial scalar write attempt
- invalid stored bool byte
- unsupported float configuration
- fingerprint mismatch after schema changes
- CRC-correct but semantically invalid stored data
- one corrupt slot with one valid slot
- both slots invalid so defaults are reloaded
- callback recursion into the same context
- C++ `offsetof` on non-standard-layout types
- CMake package not found because `CMAKE_PREFIX_PATH` is unset
