# Changelog

## [1.0.0] - 2026-03-20

### Added

- Schema-driven configuration with compile-time field descriptors.
- `MCONF_ENTRY` macro for auto-computed offset and size.
- Type-safe getters/setters for bool, u8, u16, u32, i32, float, string.
- CRC32 validation (bitwise, no lookup table - saves 1 KB ROM).
- Magic + version header for corruption and migration detection.
- Automatic default fallback on any load failure.
- Key-based lookup (`mconf_find`) for runtime field access.
- Schema validation: overlap detection, type/size consistency, bounds.
- Data validation: NUL-terminated string check.
- Platform I/O callbacks with optional erase support.
- Full documentation: API reference, design rationale, porting guide.
- Test suite: 40 tests covering all types, CRC, save/load, edge cases.
- Platform recipes for ESP32, STM32, Linux, Arduino.
