/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2012, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-11-20     Bernard    the first version
 */

#include <rthw.h>
#include <rtthread.h>

#include "board.h"

#define TIMER_BASE 0x01c20c00

typedef struct st_timer {
    unsigned int tmr_irq_en;        //Timer IRQ Enable Register
    unsigned int tmr_irq_sta;       //Timer Status Register
    unsigned int dumy0;
    unsigned int dumy1;
    unsigned int tmr0_ctrl;        //Timer 0 Control Register
    unsigned int tmr0_intv_value;  //Timer 0 Interval Value Register
    unsigned int tmr0_cur_value;   //Timer 0 Current Value Register
    unsigned int dumy2;
    unsigned int tmr1_ctrl;        //Timer 1 Control Register
    unsigned int tmr1_intv_value;  //Timer 1 Interval Value Register
    unsigned int tmr1_cur_value;   //Timer 1 Current Value Register
} sunxi_timer_t;

#define h3_timer ((volatile sunxi_timer_t *)TIMER_BASE)

//tmr_irq_en
//[1]
#define TMR0_IRQ_EN (1 << 0)
//[0]
#define TMR1_IRQ_EN (1 << 1)

//tmr_irq_sta bit
//[1]
#define TMR0_IRQ_PEND (1 << 0)
//[0]
#define TMR1_IRQ_PEND (1 << 1) 

//tmrx_ctrl
//TMR_MODE bit [7]
#define MODE_CONTINUOUS (0 << 7)
#define MODE_SINGLE     (1 << 7)
//TMR_CLK_PERS [6:4]
#define DIV1   (0 << 4)
#define DIV2   (1 << 4)
#define DIV4   (2 << 4)
#define DIV8   (3 << 4)
#define DIV16  (4 << 4)
#define DIV32  (5 << 4)
#define DIV64  (6 << 4)
#define DIV128 (7 << 4)
//TMR_CLK_SRC [3:2]
#define OSC    (0 << 2)
#define OSC24M (1 << 2)
//TMR_RELOAD  [1]
#define INTERVAL_RELOAD         (1 << 1)
#define INTERVAL_RELOAD_MASK    (1 << 1)
//TMR_EN      [0]
#define TMR_STOP    (0 << 0)
#define TMR_START   (1 << 0)

#define IRQ_ARM_TIMER 50

static void timer0_init(void)
{
    h3_timer->tmr0_intv_value = 240000;
    h3_timer->tmr0_ctrl = MODE_CONTINUOUS | DIV1 | OSC24M;
    h3_timer->tmr0_ctrl =  h3_timer->tmr0_ctrl | INTERVAL_RELOAD;
    while (h3_timer->tmr0_ctrl & INTERVAL_RELOAD_MASK)
        ;
    h3_timer->tmr0_ctrl = h3_timer->tmr0_ctrl | TMR_START;
    h3_timer->tmr_irq_en = TMR0_IRQ_EN;
}

static void rt_hw_timer_isr(int vector, void *parameter)
{
    h3_timer->tmr_irq_sta = TMR0_IRQ_PEND; //write 1 to clear irq

    rt_tick_increase();
}

int rt_hw_timer_init(void)
{
    timer0_init();
    rt_hw_interrupt_install(IRQ_ARM_TIMER, rt_hw_timer_isr, RT_NULL, "tick");
    rt_hw_interrupt_umask(IRQ_ARM_TIMER);

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_timer_init);

void idle_wfi(void)
{
    asm volatile ("wfi");
}

/**
 * This function will initialize beaglebone board
 */
void rt_hw_board_init(void)
{
    /* initialzie hardware interrupt */
    rt_hw_interrupt_init();
    rt_system_heap_init(HEAP_BEGIN, HEAP_END);

    rt_components_board_init();
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);

    rt_thread_idle_sethook(idle_wfi);
}

