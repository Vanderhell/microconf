# Porting Guide

## Backend Contract

Provide caller-owned callbacks in `mconf_io_t`:

- `read`
- `write`
- `erase`
- `storage_size`
- `slot_size`
- `callback_ctx`

The library expects two slots inside the provided storage region.

## Platform Notes

- ESP32 NVS: use bounded read-modify-write blobs or two-key semantics.
- STM32 flash: check erase granularity, alignment, and bounds before programming.
- POSIX file backend: use durable temp-file plus replace semantics or document
  the crash-consistency limit clearly.
- Arduino / EEPROM: bound-check addresses and keep schema declarations
  C++-compatible.

## C++ Consumers

Schema macros avoid designated initializers, but `offsetof` still requires a
standard-layout type in C++.
