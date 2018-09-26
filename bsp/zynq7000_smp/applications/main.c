#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "rtthread.h"
#include "rthw.h"

int main(void)
{
    rt_uint32_t level;

    level = rt_hw_interrupt_disable();
    rt_kprintf("hello rt-thread\n");
    rt_hw_interrupt_enable(level);

    return 0;
}

