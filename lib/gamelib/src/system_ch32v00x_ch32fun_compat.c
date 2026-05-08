#include <stdint.h>

#include "system_ch32v00x.h"

#include "funconfig.h"

uint32_t SystemCoreClock = FUNCONF_SYSTEM_CORE_CLOCK;

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = FUNCONF_SYSTEM_CORE_CLOCK;
}
