# Porting Guide

microconf needs only two files (`mconf.h`, `mconf.c`) and a C99 compiler.
You provide read/write callbacks for your storage backend.

---

## Platform recipes

### ESP32 (ESP-IDF NVS)

```c
#include "mconf.h"
#include "nvs_flash.h"

#define CONFIG_NVS_KEY "device_cfg"

/* NVS stores blobs by key - we use offset 0 and read/write the whole thing */
static nvs_handle_t nvs;

static int nvs_read(uint32_t offset, void *buf, uint32_t len) {
    /* Read the entire blob, then copy the requested range */
    uint8_t tmp[256];
    size_t size = sizeof(tmp);
    esp_err_t err = nvs_get_blob(nvs, CONFIG_NVS_KEY, tmp, &size);
    if (err != ESP_OK) return -1;
    if (offset + len > size) return -1;
    memcpy(buf, tmp + offset, len);
    return 0;
}

static int nvs_write(uint32_t offset, const void *buf, uint32_t len) {
    /* For simplicity: read-modify-write the entire blob */
    uint8_t tmp[256];
    size_t size = sizeof(tmp);
    nvs_get_blob(nvs, CONFIG_NVS_KEY, tmp, &size); /* may fail on first write */
    memcpy(tmp + offset, buf, len);
    size = offset + len;
    if (nvs_set_blob(nvs, CONFIG_NVS_KEY, tmp, size) != ESP_OK) return -1;
    if (nvs_commit(nvs) != ESP_OK) return -1;
    return 0;
}

static const mconf_io_t flash_io = { .read = nvs_read, .write = nvs_write, .erase = NULL };
```

### STM32 (internal flash)

```c
#include "mconf.h"
#include "stm32f4xx_hal.h"

#define CONFIG_FLASH_ADDR  0x08060000   /* last sector */
#define CONFIG_FLASH_SIZE  0x20000

static int flash_read(uint32_t offset, void *buf, uint32_t len) {
    memcpy(buf, (void *)(CONFIG_FLASH_ADDR + offset), len);
    return 0;
}

static int flash_write(uint32_t offset, const void *buf, uint32_t len) {
    HAL_FLASH_Unlock();
    const uint8_t *p = (const uint8_t *)buf;
    for (uint32_t i = 0; i < len; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,
                              CONFIG_FLASH_ADDR + offset + i, p[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }
    }
    HAL_FLASH_Lock();
    return 0;
}

static int flash_erase(uint32_t offset, uint32_t len) {
    (void)offset; (void)len;
    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .Sector = FLASH_SECTOR_7,
        .NbSectors = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,
    };
    uint32_t error;
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef rc = HAL_FLASHEx_Erase(&erase, &error);
    HAL_FLASH_Lock();
    return (rc == HAL_OK) ? 0 : -1;
}

static const mconf_io_t flash_io = {
    .read = flash_read, .write = flash_write, .erase = flash_erase
};
```

### Linux / POSIX (file-based)

```c
#include "mconf.h"
#include <stdio.h>

#define CONFIG_FILE "/etc/mydevice/config.bin"

static int file_read(uint32_t offset, void *buf, uint32_t len) {
    FILE *f = fopen(CONFIG_FILE, "rb");
    if (!f) return -1;
    fseek(f, offset, SEEK_SET);
    size_t n = fread(buf, 1, len, f);
    fclose(f);
    return (n == len) ? 0 : -1;
}

static int file_write(uint32_t offset, const void *buf, uint32_t len) {
    FILE *f = fopen(CONFIG_FILE, "r+b");
    if (!f) f = fopen(CONFIG_FILE, "wb");
    if (!f) return -1;
    fseek(f, offset, SEEK_SET);
    size_t n = fwrite(buf, 1, len, f);
    fclose(f);
    return (n == len) ? 0 : -1;
}

static const mconf_io_t file_io = { .read = file_read, .write = file_write, .erase = NULL };
```

### Arduino (EEPROM)

```cpp
extern "C" {
    #include "mconf.h"
}
#include <EEPROM.h>

static int eeprom_read(uint32_t offset, void *buf, uint32_t len) {
    uint8_t *p = (uint8_t *)buf;
    for (uint32_t i = 0; i < len; i++) p[i] = EEPROM.read(offset + i);
    return 0;
}

static int eeprom_write(uint32_t offset, const void *buf, uint32_t len) {
    const uint8_t *p = (const uint8_t *)buf;
    for (uint32_t i = 0; i < len; i++) EEPROM.write(offset + i, p[i]);
    return 0;
}

static const mconf_io_t eeprom_io = { .read = eeprom_read, .write = eeprom_write, .erase = NULL };
```

---

## CMake integration

```cmake
add_library(microconf STATIC lib/microconf/src/mconf.c)
target_include_directories(microconf PUBLIC lib/microconf/include)
target_link_libraries(my_app PRIVATE microconf)
```

---

## Checklist for a new platform

1. **C99 compiler?** -> good to go.
2. **Provide `read` callback** -> read N bytes from offset.
3. **Provide `write` callback** -> write N bytes to offset.
4. **Need erase before write?** -> provide `erase` callback (flash). Otherwise NULL.
5. **Multiple threads?** -> protect `mconf_load`/`mconf_save` with a mutex.
6. **ROM-constrained?** -> set `MCONF_ENABLE_NAMES 0`.
