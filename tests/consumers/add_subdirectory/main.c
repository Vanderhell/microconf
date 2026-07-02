#include "mconf.h"

int main(void)
{
    return (mconf_type_name(MCONF_TYPE_U16) != 0) ? 0 : 1;
}
