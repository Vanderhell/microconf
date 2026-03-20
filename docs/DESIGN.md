# Design Rationale

Why microconf is built the way it is.

---

## 1. Schema-driven vs key-value store

**Decision:** Configuration is described by a compile-time schema that maps
field names, types, offsets, and defaults. The user defines a plain C struct
and the schema tells microconf how to interpret it.

**Alternatives considered:**

- **Key-value store** (string key -> opaque blob). Flexible, but requires
  dynamic memory for variable-length values, hash maps or linear search
  for lookup, and manual type casting everywhere.
- **JSON/INI parser.** Human-readable but expensive to parse on MCUs (both
  RAM and CPU). No type safety.
- **Protobuf / FlatBuffers.** Good, but require code generation and
  external tools. Overkill for 10-20 config fields.

**Why schema wins for embedded:**

- The schema is `const` / ROM-safe. On a 32 KB flash MCU, every byte of
  RAM matters.
- The user's struct gives them direct, zero-overhead field access:
  `cfg.mqtt_port` instead of `config_get_u16("mqtt_port")`.
- Type safety: the typed getters catch mismatches at runtime (and at
  review time - the enum index makes the type obvious).
- Adding a field is a one-line struct change + one-line schema change.

**Tradeoff accepted:** The schema must match the struct exactly. If they
get out of sync, `mconf_schema_validate()` catches it - but only at
runtime, not at compile time.

---

## 2. CRC32 without lookup table

**Decision:** Use bitwise CRC32 computation (8 iterations per byte) rather
than a 256-entry lookup table.

**Why:**

- The lookup table is 1024 bytes of ROM. On a Cortex-M0 with 16 KB flash,
  that's 6% of total flash for one function.
- Config operations (load/save) happen at boot and on user action - maybe
  once per minute at most. The speed difference (3 us vs 50 us for a
  200-byte config) is irrelevant.
- The bitwise implementation is 15 lines of code, easy to audit, and
  produces identical results (ISO 3309 / ITU-T V.42 polynomial).

**Tradeoff accepted:** Slower CRC computation. Irrelevant for typical
config sizes (< 1 KB) and access patterns (< 1 Hz).

---

## 3. Flat binary format vs structured serialization

**Decision:** The storage format is: 16-byte header + raw struct bytes.
No per-field encoding, no length prefixes, no tags.

**Why:**

- The struct layout is known at compile time from the schema. There is
  no ambiguity about where each field is - the offset and size are fixed.
- No serialization overhead: `memcpy` in, `memcpy` out. Fastest possible
  I/O.
- The CRC covers the entire data blob. Any corruption is detected.
- Version migration is handled at the header level: if the schema version
  changes, the entire config reloads from defaults. This is simple and
  correct - partial migration of binary struct layouts is fragile and
  error-prone.

**Tradeoff accepted:** No partial migration. If you add a field in
firmware v2, all v1 settings are lost. For most IoT devices, this is
acceptable - settings are few and can be reconfigured. For devices that
need lossless migration, a key-value store is a better fit.

---

## 4. Default fallback on any error

**Decision:** When `mconf_load()` detects any validation failure (bad
magic, wrong version, CRC mismatch, I/O error), it loads defaults and
returns the specific error code.

**Why:**

- The device must always have a working configuration. Hanging at boot
  because the config is corrupted is unacceptable for an IoT device.
- The caller can inspect the error code and log it, but the config struct
  is always populated and usable after `mconf_load()`.
- This is "fail-safe" behaviour: the worst case is factory defaults, not
  a crash or undefined behaviour.

**Tradeoff accepted:** Silent data loss if the config is corrupted. The
error code is returned for the caller to log - but the defaults are
applied regardless.

---

## 5. Platform abstraction via I/O callbacks

**Decision:** The user provides `read`, `write`, and optional `erase`
function pointers bundled in `mconf_io_t`.

**Why:**

Same rationale as microres: there is no universal flash API. ESP-IDF uses
NVS, STM32 uses HAL flash, Zephyr uses NVS or FCB, Linux uses files,
tests use a RAM buffer. Callbacks keep the library platform-agnostic.

- `erase` is optional because some platforms (NVS, file I/O) don't need
  explicit erasure before writing.
- The offset-based API works for both raw flash partitions and NVS-like
  key-value backends (use offset 0, read/write the whole blob).

---

## 6. MCONF_ENTRY macro

**Decision:** Provide a helper macro that uses `offsetof()` and `sizeof()`
to auto-compute field offset and size from the struct definition.

**Why:**

- Eliminates a class of bugs: manually specifying offset=96 size=4 is
  error-prone and breaks when you reorder struct fields.
- The macro ensures the schema always matches the struct layout.
- Designated initializers (`.key = #field`) auto-generate the key name
  from the field name - one less thing to keep in sync.

---

## 7. String handling

**Decision:** Strings are fixed-size `char[]` arrays in the config struct.
`mconf_set_str()` always NUL-terminates. `mconf_validate()` checks that
all string fields have a NUL byte within their allocated size.

**Why:**

- Fixed-size eliminates dynamic allocation.
- Guaranteed NUL termination eliminates buffer overrun bugs.
- Truncation on write is explicit and predictable.
- Validation catches corrupted flash that might have lost the NUL byte.

**Tradeoff accepted:** Wasted space if most strings are shorter than the
allocated size. For config (SSID <= 32, hostname <= 48), this is fine.

---

## Summary of tradeoffs

| Decision | Gains | Costs |
|----------|-------|-------|
| Schema-driven | Type-safe, ROM-friendly, fast | Must keep schema + struct in sync |
| Bitwise CRC | Saves 1 KB ROM | Slower (irrelevant at config scale) |
| Flat binary | Zero serialization overhead | No partial version migration |
| Default fallback | Always-valid config | Silent reset on corruption |
| I/O callbacks | Any platform | Extra function pointers |
| Fixed-size strings | No allocation, safe | Wastes bytes on short strings |
