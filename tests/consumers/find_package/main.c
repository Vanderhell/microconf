#include "mconf.h"

int main(void)
{
    return (mconf_err_str(MCONF_OK) != 0) ? 0 : 1;
}
