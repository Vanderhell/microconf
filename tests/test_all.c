/*
 * Repo-local smoke harness for the validated microconf API.
 *
 * The prompt pack for this audit forbids running tests automatically here.
 * Keep this file C99-only and deterministic for CI/manual use.
 */

#include "mconf.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t enabled;
    uint16_t interval_ms;
    char name[12];
    uint8_t key[4];
} demo_config_t;

static const uint16_t default_interval_ms = 250u;
static const uint8_t default_key[4] = { 1u, 2u, 3u, 4u };

static const mconf_entry_t demo_entries[] = {
    MCONF_ENTRY_SCALAR(demo_config_t, enabled, MCONF_TYPE_BOOL, &(const uint8_t){ 1u }),
    MCONF_ENTRY_SCALAR(demo_config_t, interval_ms, MCONF_TYPE_U16, &default_interval_ms),
    MCONF_ENTRY_STRING(demo_config_t, name, "node-a"),
    MCONF_ENTRY_BLOB(demo_config_t, key, default_key, sizeof(default_key))
};

static const mconf_schema_t demo_schema = {
    demo_entries,
    sizeof(demo_entries) / sizeof(demo_entries[0]),
    1u,
    sizeof(demo_config_t)
};

typedef struct {
    uint8_t bytes[256];
    int fail_after_write;
    int writes;
} fake_store_t;

static int fake_read(void *ctx, size_t offset, void *buffer, size_t size)
{
    fake_store_t *store = (fake_store_t *)ctx;
    if ((offset + size) > sizeof(store->bytes)) {
        return -1;
    }
    memcpy(buffer, &store->bytes[offset], size);
    return 0;
}

static int fake_write(void *ctx, size_t offset, const void *buffer, size_t size)
{
    fake_store_t *store = (fake_store_t *)ctx;
    if ((offset + size) > sizeof(store->bytes)) {
        return -1;
    }
    if ((store->fail_after_write >= 0) && (store->writes >= store->fail_after_write)) {
        return -1;
    }
    memcpy(&store->bytes[offset], buffer, size);
    store->writes += 1;
    return 0;
}

static int fake_erase(void *ctx, size_t offset, size_t size)
{
    fake_store_t *store = (fake_store_t *)ctx;
    if ((offset + size) > sizeof(store->bytes)) {
        return -1;
    }
    memset(&store->bytes[offset], 0xFF, size);
    return 0;
}

static void must(int condition, const char *message, int *failures)
{
    if (!condition) {
        printf("FAIL: %s\n", message);
        *failures += 1;
    }
}

int main(void)
{
    demo_config_t cfg;
    demo_config_t loaded;
    mconf_t ctx;
    mconf_t other_ctx;
    fake_store_t store;
    mconf_io_t io;
    size_t required = 0u;
    int failures = 0;

    memset(&cfg, 0xCC, sizeof(cfg));
    memset(&loaded, 0xCC, sizeof(loaded));
    memset(&store, 0xFF, sizeof(store));
    store.fail_after_write = -1;
    store.writes = 0;

    io.callback_ctx = &store;
    io.storage_size = sizeof(store.bytes);
    io.slot_size = sizeof(store.bytes) / 2u;
    io.read = fake_read;
    io.write = fake_write;
    io.erase = fake_erase;

    must(mconf_init(&ctx, sizeof(ctx), &demo_schema, &cfg, sizeof(cfg)) == MCONF_OK, "init ctx", &failures);
    must(mconf_init(&other_ctx, sizeof(other_ctx), &demo_schema, &loaded, sizeof(loaded)) == MCONF_OK, "init other ctx", &failures);
    must(mconf_load_defaults(&ctx) == MCONF_OK, "defaults", &failures);
    must(cfg.enabled == 1u, "bool default", &failures);
    must(cfg.interval_ms == default_interval_ms, "u16 default", &failures);
    must(strcmp(cfg.name, "node-a") == 0, "string default", &failures);
    must(memcmp(cfg.key, default_key, sizeof(cfg.key)) == 0, "blob default", &failures);

    must(mconf_set_string(&ctx, 2u, "node-b", 6u, &required) == MCONF_OK, "set string", &failures);
    must(required == 7u, "string required capacity", &failures);
    must(mconf_set_u16(&ctx, 1u, 500u) == MCONF_OK, "set scalar", &failures);
    must(mconf_save(&ctx, &io) == MCONF_OK, "save committed slot", &failures);
    must(mconf_load(&other_ctx, &io) == MCONF_OK, "load newest slot", &failures);
    must(other_ctx.schema_fingerprint == ctx.schema_fingerprint, "fingerprint stable", &failures);
    must(loaded.interval_ms == 500u, "loaded scalar", &failures);
    must(strcmp(loaded.name, "node-b") == 0, "loaded string", &failures);

    memset(&loaded, 0, sizeof(loaded));
    store.fail_after_write = 1;
    must(mconf_set_u16(&ctx, 1u, 900u) == MCONF_OK, "set next scalar", &failures);
    must(mconf_save(&ctx, &io) != MCONF_OK, "fault injected save", &failures);
    must(mconf_load(&other_ctx, &io) == MCONF_OK, "previous committed slot survives", &failures);
    must(loaded.interval_ms == 500u, "old slot retained", &failures);

    printf("microconf smoke harness: %s\n", failures == 0 ? "PASS" : "FAIL");
    return failures == 0 ? 0 : 1;
}
