# microconf

[![CI](https://github.com/Vanderhell/microconf/actions/workflows/ci.yml/badge.svg)](https://github.com/Vanderhell/microconf/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![C99](https://img.shields.io/badge/language-C99-blue.svg)](#)
[![Docs](https://img.shields.io/badge/docs-complete-black.svg)](docs/API_REFERENCE.md)

Type-safe configuration manager for embedded systems.

`C99` `Zero dependencies` `Zero allocations` `Flash-friendly` `Portable`

## Overview

Every embedded project eventually needs configuration storage: Wi-Fi credentials,
MQTT endpoints, calibration values, feature flags, and factory defaults.
`microconf` provides a small schema-driven layer for storing and validating that
configuration without pulling in JSON, dynamic allocation, or platform lock-in.

It is designed for firmware and low-level C environments where binary size,
predictability, and portability matter more than dynamic features.

## Highlights

- Schema-driven configuration defined as a `const` array.
- Type-safe getters and setters for common scalar types and fixed strings.
- CRC32 validation on load/save with automatic fallback to defaults.
- Versioned on-storage header to detect incompatible schema changes.
- Portable read/write/erase callbacks for flash, EEPROM, NVS, or files.
- No dynamic allocation and no external dependencies.
- Runtime validation for schema consistency and stored data integrity.

## Quick Example

```c
#include "mconf.h"

typedef struct {
    char     wifi_ssid[32];
    uint16_t mqtt_port;
    float    sensor_cal;
    bool     debug_mode;
} my_config_t;

static const uint16_t DEF_PORT = 1883;
static const float DEF_CAL = 1.0f;
static const bool DEF_DEBUG = false;

static const mconf_entry_t entries[] = {
    MCONF_ENTRY(my_config_t, wifi_ssid,  MCONF_TYPE_STR,   NULL),
    MCONF_ENTRY(my_config_t, mqtt_port,  MCONF_TYPE_U16,   &DEF_PORT),
    MCONF_ENTRY(my_config_t, sensor_cal, MCONF_TYPE_FLOAT, &DEF_CAL),
    MCONF_ENTRY(my_config_t, debug_mode, MCONF_TYPE_BOOL,  &DEF_DEBUG),
};

static const mconf_schema_t schema = {
    .entries = entries,
    .num_entries = 4,
    .version = 1,
    .data_size = sizeof(my_config_t),
};

my_config_t cfg;
mconf_load(&schema, &cfg, &flash_io);
mconf_set_u16(&schema, &cfg, 1, 8883);
mconf_save(&schema, &cfg, &flash_io);
```

## Features

- **Small footprint**: compact C99 implementation suitable for embedded targets.
- **Portable storage layer**: storage backend is defined by your callbacks.
- **Safe defaults**: invalid, empty, or corrupted storage falls back cleanly.
- **Field lookup support**: optional key-name lookup for CLI or remote config flows.
- **ROM-conscious design**: schema can live in flash/ROM and name lookup can be disabled.

## Build And Test

On Linux or macOS with `gcc` and `make` installed:

```sh
cd tests
make
```

The test binary is compiled with:

- `-std=c99`
- `-Wall`
- `-Wextra`
- `-Wpedantic`
- `-Werror`

## Integration Layout

```text
your_project/
|-- lib/
|   `-- microconf/
|       |-- include/
|       |   `-- mconf.h
|       `-- src/
|           `-- mconf.c
```

## Configuration Knobs

| Macro | Default | Purpose |
|---|---:|---|
| `MCONF_MAX_ENTRIES` | `32` | Maximum schema entry count |
| `MCONF_MAX_KEY_LEN` | `24` | Maximum stored key length |
| `MCONF_ENABLE_NAMES` | `1` | Enables runtime key-name lookup |
| `MCONF_ASSERT(expr)` | none | Project-specific assert hook |

## API Snapshot

| Function | Purpose |
|---|---|
| `mconf_load_defaults` | Populate a config struct from schema defaults |
| `mconf_load` | Load, validate, and recover from invalid storage |
| `mconf_save` | Persist config and compute CRC |
| `mconf_validate` | Validate stored config data |
| `mconf_find` | Find a field by key name |
| `mconf_get_*` / `mconf_set_*` | Typed field access |
| `mconf_schema_validate` | Validate schema layout and metadata |
| `mconf_crc32` | CRC32 helper |

## Documentation

- [API reference](docs/API_REFERENCE.md)
- [Design rationale](docs/DESIGN.md)
- [Porting guide](docs/PORTING_GUIDE.md)
- [Contributing guide](CONTRIBUTING.md)
- [Changelog](CHANGELOG.md)

## Repository Structure

```text
microconf/
|-- include/
|   `-- mconf.h
|-- src/
|   `-- mconf.c
|-- tests/
|   |-- Makefile
|   `-- test_all.c
|-- docs/
|   |-- API_REFERENCE.md
|   |-- DESIGN.md
|   `-- PORTING_GUIDE.md
|-- .github/workflows/ci.yml
|-- CHANGELOG.md
|-- CONTRIBUTING.md
|-- LICENSE
`-- README.md
```

## Ecosystem

- [microfsm](https://github.com/Vanderhell/microfsm): state machine integration for boot and runtime flows.
- [microres](https://github.com/Vanderhell/microres): fault tolerance and retry patterns around config persistence.
- [iotspool](https://github.com/Vanderhell/iotspool): message-driven systems that consume runtime configuration.

## License

Released under the MIT License.

Copyright (c) 2026 Vanderhell
