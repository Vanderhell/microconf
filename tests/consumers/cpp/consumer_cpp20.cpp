extern "C" {
#include "mconf.h"
}

struct CppConfig20 {
    unsigned char enabled;
    char name[8];
};

static const unsigned char kEnabled20 = 1u;
static const mconf_entry_t kEntries20[] = {
    MCONF_ENTRY_SCALAR(CppConfig20, enabled, MCONF_TYPE_BOOL, &kEnabled20),
    MCONF_ENTRY_STRING(CppConfig20, name, "cpp20")
};

int main()
{
    return (kEntries20[0].default_size == sizeof(kEnabled20)) ? 0 : 1;
}
