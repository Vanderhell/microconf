extern "C" {
#include "mconf.h"
}

struct CppConfig11 {
    unsigned short port;
    char name[8];
};

static const unsigned short kPort11 = 1883u;
static const mconf_entry_t kEntries11[] = {
    MCONF_ENTRY_SCALAR(CppConfig11, port, MCONF_TYPE_U16, &kPort11),
    MCONF_ENTRY_STRING(CppConfig11, name, "cpp11")
};

int main()
{
    return (kEntries11[0].offset == offsetof(CppConfig11, port)) ? 0 : 1;
}
