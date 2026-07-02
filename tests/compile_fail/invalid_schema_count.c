#include "mconf.h"

typedef struct {
    uint8_t value;
} demo_t;

static const uint8_t default_value = 1u;
static const mconf_entry_t entries[] = {
    MCONF_ENTRY_SCALAR(demo_t, value, MCONF_TYPE_U8, &default_value)
};

static const mconf_schema_t schema = {
    entries,
    (size_t)(MCONF_MAX_ENTRIES + 1),
    1u,
    sizeof(demo_t)
};

int main(void)
{
    return (int)schema.entry_count;
}
