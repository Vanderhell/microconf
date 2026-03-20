/*
 * microconf test suite.
 *
 * Build: gcc -std=c99 -Wall -Wextra -I../include ../src/mconf.c test_all.c -o test_all
 * Run:   ./test_all
 */

#include "mconf.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* -- Minimal test framework ---------------------------------------------- */

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define TEST(name) static void name(void)
#define RUN_TEST(name) do {                                     \
    tests_run++;                                                \
    printf("  %-55s ", #name);                                  \
    name();                                                     \
    printf("PASS\n");                                           \
    tests_passed++;                                             \
} while (0)

#define ASSERT_EQ(expected, actual) do {                        \
    if ((expected) != (actual)) {                               \
        printf("FAIL\n    %s:%d: expected %d, got %d\n",       \
               __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_TRUE(expr) do {                                  \
    if (!(expr)) {                                              \
        printf("FAIL\n    %s:%d: expected true\n",              \
               __FILE__, __LINE__);                             \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_FALSE(expr) do {                                 \
    if ((expr)) {                                               \
        printf("FAIL\n    %s:%d: expected false\n",             \
               __FILE__, __LINE__);                             \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_STR_EQ(expected, actual) do {                    \
    if (strcmp((expected), (actual)) != 0) {                     \
        printf("FAIL\n    %s:%d: expected \"%s\", got \"%s\"\n",\
               __FILE__, __LINE__, (expected), (actual));       \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_FLOAT_EQ(expected, actual) do {                  \
    if (fabsf((expected) - (actual)) > 0.001f) {                \
        printf("FAIL\n    %s:%d: expected %f, got %f\n",       \
               __FILE__, __LINE__, (double)(expected), (double)(actual)); \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

/* -- Test config struct -------------------------------------------------- */

typedef struct {
    char     wifi_ssid[32];
    char     wifi_pass[64];
    char     mqtt_host[48];
    uint16_t mqtt_port;
    bool     mqtt_tls;
    uint8_t  log_level;
    int32_t  timezone_offset;
    float    sensor_cal;
    uint32_t report_interval_ms;
} test_config_t;

/* -- Defaults ------------------------------------------------------------ */

static const char     DEF_SSID[]     = "MyNetwork";
static const char     DEF_PASS[]     = "";
static const char     DEF_HOST[]     = "mqtt.example.com";
static const uint16_t DEF_PORT      = 1883;
static const bool     DEF_TLS       = false;
static const uint8_t  DEF_LOG       = 2;
static const int32_t  DEF_TZ        = 0;
static const float    DEF_CAL       = 1.0f;
static const uint32_t DEF_INTERVAL  = 60000;

/* -- Schema -------------------------------------------------------------- */

enum {
    CFG_WIFI_SSID,
    CFG_WIFI_PASS,
    CFG_MQTT_HOST,
    CFG_MQTT_PORT,
    CFG_MQTT_TLS,
    CFG_LOG_LEVEL,
    CFG_TZ_OFFSET,
    CFG_SENSOR_CAL,
    CFG_REPORT_INTERVAL,
    CFG_COUNT
};

static const mconf_entry_t test_entries[] = {
    MCONF_ENTRY(test_config_t, wifi_ssid,           MCONF_TYPE_STR,   DEF_SSID),
    MCONF_ENTRY(test_config_t, wifi_pass,           MCONF_TYPE_STR,   DEF_PASS),
    MCONF_ENTRY(test_config_t, mqtt_host,           MCONF_TYPE_STR,   DEF_HOST),
    MCONF_ENTRY(test_config_t, mqtt_port,           MCONF_TYPE_U16,   &DEF_PORT),
    MCONF_ENTRY(test_config_t, mqtt_tls,            MCONF_TYPE_BOOL,  &DEF_TLS),
    MCONF_ENTRY(test_config_t, log_level,           MCONF_TYPE_U8,    &DEF_LOG),
    MCONF_ENTRY(test_config_t, timezone_offset,     MCONF_TYPE_I32,   &DEF_TZ),
    MCONF_ENTRY(test_config_t, sensor_cal,          MCONF_TYPE_FLOAT, &DEF_CAL),
    MCONF_ENTRY(test_config_t, report_interval_ms,  MCONF_TYPE_U32,   &DEF_INTERVAL),
};

static const mconf_schema_t test_schema = {
    .entries     = test_entries,
    .num_entries = CFG_COUNT,
    .version     = 1,
    .data_size   = sizeof(test_config_t),
};

/* -- Mock flash storage -------------------------------------------------- */

#define FLASH_SIZE 512
static uint8_t mock_flash[FLASH_SIZE];
static bool io_fail_read  = false;
static bool io_fail_write = false;
static bool io_fail_erase = false;

static void reset_flash(void) {
    memset(mock_flash, 0xFF, FLASH_SIZE);
    io_fail_read  = false;
    io_fail_write = false;
    io_fail_erase = false;
}

static int mock_read(uint32_t offset, void *buf, uint32_t len) {
    if (io_fail_read) return -1;
    if (offset + len > FLASH_SIZE) return -1;
    memcpy(buf, mock_flash + offset, len);
    return 0;
}

static int mock_write(uint32_t offset, const void *buf, uint32_t len) {
    if (io_fail_write) return -1;
    if (offset + len > FLASH_SIZE) return -1;
    memcpy(mock_flash + offset, buf, len);
    return 0;
}

static int mock_erase(uint32_t offset, uint32_t len) {
    if (io_fail_erase) return -1;
    if (offset + len > FLASH_SIZE) return -1;
    memset(mock_flash + offset, 0xFF, len);
    return 0;
}

static const mconf_io_t test_io = {
    .read  = mock_read,
    .write = mock_write,
    .erase = mock_erase,
};

static const mconf_io_t test_io_no_erase = {
    .read  = mock_read,
    .write = mock_write,
    .erase = NULL,
};

/* ===========================================================================
 * Tests: Defaults
 * =========================================================================== */

TEST(test_load_defaults) {
    test_config_t cfg;
    ASSERT_EQ(MCONF_OK, mconf_load_defaults(&test_schema, &cfg));
    ASSERT_STR_EQ("MyNetwork", cfg.wifi_ssid);
    ASSERT_STR_EQ("", cfg.wifi_pass);
    ASSERT_STR_EQ("mqtt.example.com", cfg.mqtt_host);
    ASSERT_EQ(1883, cfg.mqtt_port);
    ASSERT_FALSE(cfg.mqtt_tls);
    ASSERT_EQ(2, cfg.log_level);
    ASSERT_EQ(0, cfg.timezone_offset);
    ASSERT_FLOAT_EQ(1.0f, cfg.sensor_cal);
    ASSERT_EQ(60000, (int)cfg.report_interval_ms);
}

TEST(test_load_defaults_null) {
    test_config_t cfg;
    ASSERT_EQ(MCONF_ERR_NULL, mconf_load_defaults(NULL, &cfg));
    ASSERT_EQ(MCONF_ERR_NULL, mconf_load_defaults(&test_schema, NULL));
}

/* ===========================================================================
 * Tests: Save and Load
 * =========================================================================== */

TEST(test_save_and_load) {
    reset_flash();
    test_config_t cfg, loaded;

    mconf_load_defaults(&test_schema, &cfg);
    /* Modify some values */
    strcpy(cfg.wifi_ssid, "TestWiFi");
    cfg.mqtt_port = 8883;
    cfg.mqtt_tls = true;
    cfg.sensor_cal = 2.5f;

    ASSERT_EQ(MCONF_OK, mconf_save(&test_schema, &cfg, &test_io));
    ASSERT_EQ(MCONF_OK, mconf_load(&test_schema, &loaded, &test_io));

    ASSERT_STR_EQ("TestWiFi", loaded.wifi_ssid);
    ASSERT_EQ(8883, loaded.mqtt_port);
    ASSERT_TRUE(loaded.mqtt_tls);
    ASSERT_FLOAT_EQ(2.5f, loaded.sensor_cal);
    ASSERT_STR_EQ("mqtt.example.com", loaded.mqtt_host);  /* unchanged */
}

TEST(test_save_without_erase) {
    reset_flash();
    test_config_t cfg, loaded;
    mconf_load_defaults(&test_schema, &cfg);

    ASSERT_EQ(MCONF_OK, mconf_save(&test_schema, &cfg, &test_io_no_erase));
    ASSERT_EQ(MCONF_OK, mconf_load(&test_schema, &loaded, &test_io_no_erase));
    ASSERT_STR_EQ("MyNetwork", loaded.wifi_ssid);
}

TEST(test_load_empty_flash_returns_defaults) {
    reset_flash();
    test_config_t cfg;
    /* Flash is 0xFF - no valid header */
    mconf_err_t err = mconf_load(&test_schema, &cfg, &test_io);
    ASSERT_EQ(MCONF_ERR_MAGIC, err);
    /* Should have loaded defaults */
    ASSERT_STR_EQ("MyNetwork", cfg.wifi_ssid);
    ASSERT_EQ(1883, cfg.mqtt_port);
}

TEST(test_load_corrupted_crc) {
    reset_flash();
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    mconf_save(&test_schema, &cfg, &test_io);

    /* Corrupt one data byte */
    mock_flash[sizeof(mconf_header_t) + 5] ^= 0xAA;

    test_config_t loaded;
    mconf_err_t err = mconf_load(&test_schema, &loaded, &test_io);
    ASSERT_EQ(MCONF_ERR_CRC, err);
    /* Defaults loaded on CRC failure */
    ASSERT_STR_EQ("MyNetwork", loaded.wifi_ssid);
}

TEST(test_load_wrong_version) {
    reset_flash();
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    mconf_save(&test_schema, &cfg, &test_io);

    /* Change version in header */
    mconf_header_t *h = (mconf_header_t *)mock_flash;
    h->version = 99;

    test_config_t loaded;
    mconf_err_t err = mconf_load(&test_schema, &loaded, &test_io);
    ASSERT_EQ(MCONF_ERR_VERSION, err);
}

TEST(test_save_null) {
    ASSERT_EQ(MCONF_ERR_NULL, mconf_save(NULL, NULL, NULL));
}

TEST(test_load_null) {
    test_config_t cfg;
    ASSERT_EQ(MCONF_ERR_NULL, mconf_load(NULL, &cfg, &test_io));
    ASSERT_EQ(MCONF_ERR_NULL, mconf_load(&test_schema, NULL, &test_io));
    ASSERT_EQ(MCONF_ERR_NULL, mconf_load(&test_schema, &cfg, NULL));
}

TEST(test_save_io_failure) {
    reset_flash();
    io_fail_write = true;
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    /* erase succeeds, write fails */
    ASSERT_EQ(MCONF_ERR_IO, mconf_save(&test_schema, &cfg, &test_io));
}

TEST(test_load_io_failure) {
    reset_flash();
    io_fail_read = true;
    test_config_t cfg;
    mconf_err_t err = mconf_load(&test_schema, &cfg, &test_io);
    ASSERT_EQ(MCONF_ERR_IO, err);
    /* Should load defaults on IO failure */
    ASSERT_STR_EQ("MyNetwork", cfg.wifi_ssid);
}

/* ===========================================================================
 * Tests: Field access
 * =========================================================================== */

TEST(test_find_by_key) {
    ASSERT_EQ(CFG_WIFI_SSID, mconf_find(&test_schema, "wifi_ssid"));
    ASSERT_EQ(CFG_MQTT_PORT, mconf_find(&test_schema, "mqtt_port"));
    ASSERT_EQ(CFG_SENSOR_CAL, mconf_find(&test_schema, "sensor_cal"));
    ASSERT_EQ(-1, mconf_find(&test_schema, "nonexistent"));
    ASSERT_EQ(-1, mconf_find(NULL, "wifi_ssid"));
    ASSERT_EQ(-1, mconf_find(&test_schema, NULL));
}

TEST(test_get_set_generic) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);

    uint16_t port;
    ASSERT_EQ(MCONF_OK, mconf_get(&test_schema, &cfg, CFG_MQTT_PORT, &port, sizeof(port)));
    ASSERT_EQ(1883, port);

    port = 9999;
    ASSERT_EQ(MCONF_OK, mconf_set(&test_schema, &cfg, CFG_MQTT_PORT, &port, sizeof(port)));
    ASSERT_EQ(9999, cfg.mqtt_port);
}

TEST(test_get_set_out_of_range) {
    test_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    uint16_t val = 0;
    ASSERT_EQ(MCONF_ERR_NOT_FOUND, mconf_get(&test_schema, &cfg, 99, &val, sizeof(val)));
    ASSERT_EQ(MCONF_ERR_NOT_FOUND, mconf_set(&test_schema, &cfg, 99, &val, sizeof(val)));
}

TEST(test_get_buffer_too_small) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    uint8_t tiny;
    ASSERT_EQ(MCONF_ERR_SIZE, mconf_get(&test_schema, &cfg, CFG_MQTT_PORT, &tiny, 1));
}

TEST(test_reset_field) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);

    cfg.mqtt_port = 9999;
    ASSERT_EQ(9999, cfg.mqtt_port);

    ASSERT_EQ(MCONF_OK, mconf_reset_field(&test_schema, &cfg, CFG_MQTT_PORT));
    ASSERT_EQ(1883, cfg.mqtt_port);
}

TEST(test_reset_field_null_default) {
    /* Entry without default should zero the field */
    static const mconf_entry_t minimal_entries[] = {
        { .key = "val", .offset = 0, .size = 4, .type = MCONF_TYPE_U32, .default_val = NULL },
    };
    static const mconf_schema_t minimal_schema = {
        .entries = minimal_entries, .num_entries = 1, .version = 1, .data_size = 4,
    };
    uint32_t data = 12345;
    ASSERT_EQ(MCONF_OK, mconf_reset_field(&minimal_schema, &data, 0));
    ASSERT_EQ(0, (int)data);
}

/* ===========================================================================
 * Tests: Typed getters/setters
 * =========================================================================== */

TEST(test_typed_bool) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    bool val;
    ASSERT_EQ(MCONF_OK, mconf_get_bool(&test_schema, &cfg, CFG_MQTT_TLS, &val));
    ASSERT_FALSE(val);
    ASSERT_EQ(MCONF_OK, mconf_set_bool(&test_schema, &cfg, CFG_MQTT_TLS, true));
    ASSERT_TRUE(cfg.mqtt_tls);
}

TEST(test_typed_u8) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    uint8_t val;
    ASSERT_EQ(MCONF_OK, mconf_get_u8(&test_schema, &cfg, CFG_LOG_LEVEL, &val));
    ASSERT_EQ(2, val);
    ASSERT_EQ(MCONF_OK, mconf_set_u8(&test_schema, &cfg, CFG_LOG_LEVEL, 5));
    ASSERT_EQ(5, cfg.log_level);
}

TEST(test_typed_u16) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    uint16_t val;
    ASSERT_EQ(MCONF_OK, mconf_get_u16(&test_schema, &cfg, CFG_MQTT_PORT, &val));
    ASSERT_EQ(1883, val);
    ASSERT_EQ(MCONF_OK, mconf_set_u16(&test_schema, &cfg, CFG_MQTT_PORT, 8883));
    ASSERT_EQ(8883, cfg.mqtt_port);
}

TEST(test_typed_u32) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    uint32_t val;
    ASSERT_EQ(MCONF_OK, mconf_get_u32(&test_schema, &cfg, CFG_REPORT_INTERVAL, &val));
    ASSERT_EQ(60000, (int)val);
    ASSERT_EQ(MCONF_OK, mconf_set_u32(&test_schema, &cfg, CFG_REPORT_INTERVAL, 5000));
    ASSERT_EQ(5000, (int)cfg.report_interval_ms);
}

TEST(test_typed_i32) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    int32_t val;
    ASSERT_EQ(MCONF_OK, mconf_get_i32(&test_schema, &cfg, CFG_TZ_OFFSET, &val));
    ASSERT_EQ(0, val);
    ASSERT_EQ(MCONF_OK, mconf_set_i32(&test_schema, &cfg, CFG_TZ_OFFSET, -3600));
    ASSERT_EQ(-3600, cfg.timezone_offset);
}

TEST(test_typed_float) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    float val;
    ASSERT_EQ(MCONF_OK, mconf_get_float(&test_schema, &cfg, CFG_SENSOR_CAL, &val));
    ASSERT_FLOAT_EQ(1.0f, val);
    ASSERT_EQ(MCONF_OK, mconf_set_float(&test_schema, &cfg, CFG_SENSOR_CAL, 3.14f));
    ASSERT_FLOAT_EQ(3.14f, cfg.sensor_cal);
}

TEST(test_typed_str) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    char buf[32];
    ASSERT_EQ(MCONF_OK, mconf_get_str(&test_schema, &cfg, CFG_WIFI_SSID, buf, sizeof(buf)));
    ASSERT_STR_EQ("MyNetwork", buf);

    ASSERT_EQ(MCONF_OK, mconf_set_str(&test_schema, &cfg, CFG_WIFI_SSID, "NewSSID"));
    ASSERT_STR_EQ("NewSSID", cfg.wifi_ssid);
}

TEST(test_typed_str_truncation) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);

    /* wifi_ssid is 32 bytes - write a long string */
    char long_str[64];
    memset(long_str, 'A', 63);
    long_str[63] = '\0';

    ASSERT_EQ(MCONF_OK, mconf_set_str(&test_schema, &cfg, CFG_WIFI_SSID, long_str));
    /* Should be truncated to 31 chars + NUL */
    ASSERT_EQ(31, (int)strlen(cfg.wifi_ssid));
    ASSERT_EQ('\0', cfg.wifi_ssid[31]);
}

TEST(test_typed_type_mismatch) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    uint32_t val;
    /* CFG_MQTT_PORT is u16, not u32 */
    ASSERT_EQ(MCONF_ERR_TYPE, mconf_get_u32(&test_schema, &cfg, CFG_MQTT_PORT, &val));
    ASSERT_EQ(MCONF_ERR_TYPE, mconf_set_u32(&test_schema, &cfg, CFG_MQTT_PORT, 42));
}

/* ===========================================================================
 * Tests: CRC32
 * =========================================================================== */

TEST(test_crc32_known_values) {
    /* "123456789" -> CRC32 = 0xCBF43926 (standard test vector) */
    const char *data = "123456789";
    uint32_t crc = mconf_crc32(data, 9);
    ASSERT_EQ((int)0xCBF43926U, (int)crc);
}

TEST(test_crc32_empty) {
    uint32_t crc = mconf_crc32("", 0);
    ASSERT_EQ((int)0x00000000U, (int)crc);
}

TEST(test_crc32_single_byte) {
    uint8_t byte = 0x00;
    uint32_t crc = mconf_crc32(&byte, 1);
    ASSERT_EQ((int)0xD202EF8DU, (int)crc);
}

/* ===========================================================================
 * Tests: Validation
 * =========================================================================== */

TEST(test_schema_validate_ok) {
    ASSERT_EQ(MCONF_OK, mconf_schema_validate(&test_schema));
}

TEST(test_schema_validate_null) {
    ASSERT_EQ(MCONF_ERR_NULL, mconf_schema_validate(NULL));
}

TEST(test_schema_validate_overflow) {
    static const mconf_entry_t bad[] = {
        { .key = "x", .offset = 100, .size = 200, .type = MCONF_TYPE_BLOB, .default_val = NULL },
    };
    mconf_schema_t bad_schema = {
        .entries = bad, .num_entries = 1, .version = 1, .data_size = 50,
    };
    ASSERT_EQ(MCONF_ERR_INVALID, mconf_schema_validate(&bad_schema));
}

TEST(test_schema_validate_type_size_mismatch) {
    static const mconf_entry_t bad[] = {
        { .key = "x", .offset = 0, .size = 2, .type = MCONF_TYPE_U32, .default_val = NULL },
    };
    mconf_schema_t bad_schema = {
        .entries = bad, .num_entries = 1, .version = 1, .data_size = 4,
    };
    ASSERT_EQ(MCONF_ERR_INVALID, mconf_schema_validate(&bad_schema));
}

TEST(test_schema_validate_overlap) {
    static const mconf_entry_t bad[] = {
        { .key = "a", .offset = 0, .size = 4, .type = MCONF_TYPE_U32, .default_val = NULL },
        { .key = "b", .offset = 2, .size = 4, .type = MCONF_TYPE_U32, .default_val = NULL },
    };
    mconf_schema_t bad_schema = {
        .entries = bad, .num_entries = 2, .version = 1, .data_size = 8,
    };
    ASSERT_EQ(MCONF_ERR_INVALID, mconf_schema_validate(&bad_schema));
}

TEST(test_schema_validate_tiny_string) {
    static const mconf_entry_t bad[] = {
        { .key = "s", .offset = 0, .size = 1, .type = MCONF_TYPE_STR, .default_val = NULL },
    };
    mconf_schema_t bad_schema = {
        .entries = bad, .num_entries = 1, .version = 1, .data_size = 1,
    };
    ASSERT_EQ(MCONF_ERR_INVALID, mconf_schema_validate(&bad_schema));
}

TEST(test_validate_unterminated_string) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);

    /* Fill wifi_ssid with non-zero bytes, no NUL */
    memset(cfg.wifi_ssid, 'X', sizeof(cfg.wifi_ssid));

    ASSERT_EQ(MCONF_ERR_INVALID, mconf_validate(&test_schema, &cfg));
}

TEST(test_validate_ok) {
    test_config_t cfg;
    mconf_load_defaults(&test_schema, &cfg);
    ASSERT_EQ(MCONF_OK, mconf_validate(&test_schema, &cfg));
}

/* ===========================================================================
 * Tests: Error strings and type names
 * =========================================================================== */

TEST(test_err_str) {
    ASSERT_STR_EQ("ok",              mconf_err_str(MCONF_OK));
    ASSERT_STR_EQ("null pointer",    mconf_err_str(MCONF_ERR_NULL));
    ASSERT_STR_EQ("crc failed",      mconf_err_str(MCONF_ERR_CRC));
    ASSERT_STR_EQ("invalid magic",   mconf_err_str(MCONF_ERR_MAGIC));
    ASSERT_STR_EQ("version mismatch",mconf_err_str(MCONF_ERR_VERSION));
    ASSERT_STR_EQ("io error",        mconf_err_str(MCONF_ERR_IO));
    ASSERT_STR_EQ("unknown error",   mconf_err_str((mconf_err_t)99));
}

TEST(test_type_name) {
    ASSERT_STR_EQ("bool",  mconf_type_name(MCONF_TYPE_BOOL));
    ASSERT_STR_EQ("u8",    mconf_type_name(MCONF_TYPE_U8));
    ASSERT_STR_EQ("u16",   mconf_type_name(MCONF_TYPE_U16));
    ASSERT_STR_EQ("float", mconf_type_name(MCONF_TYPE_FLOAT));
    ASSERT_STR_EQ("str",   mconf_type_name(MCONF_TYPE_STR));
    ASSERT_STR_EQ("?",     mconf_type_name((mconf_type_t)99));
}

/* ===========================================================================
 * Tests: Round-trip integrity
 * =========================================================================== */

TEST(test_round_trip_all_types) {
    reset_flash();
    test_config_t cfg, loaded;
    mconf_load_defaults(&test_schema, &cfg);

    /* Set every field to a non-default value */
    mconf_set_str(&test_schema, &cfg, CFG_WIFI_SSID, "RoundTrip");
    mconf_set_str(&test_schema, &cfg, CFG_WIFI_PASS, "secret123");
    mconf_set_str(&test_schema, &cfg, CFG_MQTT_HOST, "10.0.0.1");
    mconf_set_u16(&test_schema, &cfg, CFG_MQTT_PORT, 8883);
    mconf_set_bool(&test_schema, &cfg, CFG_MQTT_TLS, true);
    mconf_set_u8(&test_schema, &cfg, CFG_LOG_LEVEL, 4);
    mconf_set_i32(&test_schema, &cfg, CFG_TZ_OFFSET, -7200);
    mconf_set_float(&test_schema, &cfg, CFG_SENSOR_CAL, 0.95f);
    mconf_set_u32(&test_schema, &cfg, CFG_REPORT_INTERVAL, 5000);

    ASSERT_EQ(MCONF_OK, mconf_save(&test_schema, &cfg, &test_io));
    ASSERT_EQ(MCONF_OK, mconf_load(&test_schema, &loaded, &test_io));

    /* Verify every field */
    ASSERT_STR_EQ("RoundTrip", loaded.wifi_ssid);
    ASSERT_STR_EQ("secret123", loaded.wifi_pass);
    ASSERT_STR_EQ("10.0.0.1", loaded.mqtt_host);
    ASSERT_EQ(8883, loaded.mqtt_port);
    ASSERT_TRUE(loaded.mqtt_tls);
    ASSERT_EQ(4, loaded.log_level);
    ASSERT_EQ(-7200, loaded.timezone_offset);
    ASSERT_FLOAT_EQ(0.95f, loaded.sensor_cal);
    ASSERT_EQ(5000, (int)loaded.report_interval_ms);
}

/* ===========================================================================
 * Main
 * =========================================================================== */

int main(void) {
    printf("\n=== microconf test suite ===\n\n");

    printf("[Defaults]\n");
    RUN_TEST(test_load_defaults);
    RUN_TEST(test_load_defaults_null);

    printf("\n[Save & Load]\n");
    RUN_TEST(test_save_and_load);
    RUN_TEST(test_save_without_erase);
    RUN_TEST(test_load_empty_flash_returns_defaults);
    RUN_TEST(test_load_corrupted_crc);
    RUN_TEST(test_load_wrong_version);
    RUN_TEST(test_save_null);
    RUN_TEST(test_load_null);
    RUN_TEST(test_save_io_failure);
    RUN_TEST(test_load_io_failure);

    printf("\n[Field Access]\n");
    RUN_TEST(test_find_by_key);
    RUN_TEST(test_get_set_generic);
    RUN_TEST(test_get_set_out_of_range);
    RUN_TEST(test_get_buffer_too_small);
    RUN_TEST(test_reset_field);
    RUN_TEST(test_reset_field_null_default);

    printf("\n[Typed Getters/Setters]\n");
    RUN_TEST(test_typed_bool);
    RUN_TEST(test_typed_u8);
    RUN_TEST(test_typed_u16);
    RUN_TEST(test_typed_u32);
    RUN_TEST(test_typed_i32);
    RUN_TEST(test_typed_float);
    RUN_TEST(test_typed_str);
    RUN_TEST(test_typed_str_truncation);
    RUN_TEST(test_typed_type_mismatch);

    printf("\n[CRC32]\n");
    RUN_TEST(test_crc32_known_values);
    RUN_TEST(test_crc32_empty);
    RUN_TEST(test_crc32_single_byte);

    printf("\n[Schema Validation]\n");
    RUN_TEST(test_schema_validate_ok);
    RUN_TEST(test_schema_validate_null);
    RUN_TEST(test_schema_validate_overflow);
    RUN_TEST(test_schema_validate_type_size_mismatch);
    RUN_TEST(test_schema_validate_overlap);
    RUN_TEST(test_schema_validate_tiny_string);
    RUN_TEST(test_validate_unterminated_string);
    RUN_TEST(test_validate_ok);

    printf("\n[Error Strings]\n");
    RUN_TEST(test_err_str);
    RUN_TEST(test_type_name);

    printf("\n[Round Trip]\n");
    RUN_TEST(test_round_trip_all_types);

    printf("\n=== Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) printf(", %d FAILED", tests_failed);
    printf(" ===\n\n");

    return tests_failed > 0 ? 1 : 0;
}
