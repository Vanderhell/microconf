#include "mconf.h"

typedef struct {
    char name[4];
} invalid_t;

static const mconf_entry_t bad_entries[] = {
    MCONF_ENTRY_STRING(invalid_t, name, "toolong")
};

int main(void)
{
    return (int)bad_entries[0].default_size;
}
