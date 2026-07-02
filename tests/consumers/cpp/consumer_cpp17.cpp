extern "C" {
#include "mconf.h"
}

struct CppConfig17 {
    unsigned int interval;
    char name[8];
};

static const unsigned int kInterval17 = 100u;
static const mconf_entry_t kEntries17[] = {
    MCONF_ENTRY_SCALAR(CppConfig17, interval, MCONF_TYPE_U32, &kInterval17),
    MCONF_ENTRY_STRING(CppConfig17, name, "cpp17")
};

int main()
{
    return (kEntries17[1].type == MCONF_TYPE_STRING) ? 0 : 1;
}
