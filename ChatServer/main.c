#include <stdio.h>
#include "server.h"
#include "systemsettings.h"

int main()
{
    serverRoutine(PORT_NO, MAX_CLIENTS, ACCEPT_TIMEOUT, READ_TIMEOUT);
    return 0;
}
