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
#include "spinlock.h"

#define TIMER_LOAD(hw_base)             __REG32(hw_base + 0x00)
#define TIMER_VALUE(hw_base)            __REG32(hw_base + 0x04)
#define TIMER_CTRL(hw_base)             __REG32(hw_base + 0x08)
#define TIMER_CTRL_ONESHOT              (1 << 0)
#define TIMER_CTRL_32BIT                (1 << 1)
#define TIMER_CTRL_DIV1                 (0 << 2)
#define TIMER_CTRL_DIV16                (1 << 2)
#define TIMER_CTRL_DIV256               (2 << 2)
#define TIMER_CTRL_IE                   (1 << 5)        /* Interrupt Enable (versatile only) */
#define TIMER_CTRL_PERIODIC             (1 << 6)
#define TIMER_CTRL_ENABLE               (1 << 7)

#define TIMER_INTCLR(hw_base)           __REG32(hw_base + 0x0c)
#define TIMER_RIS(hw_base)              __REG32(hw_base + 0x10)
#define TIMER_MIS(hw_base)              __REG32(hw_base + 0x14)
#define TIMER_BGLOAD(hw_base)           __REG32(hw_base + 0x18)

#define SYS_CTRL                        __REG32(REALVIEW_SCTL_BASE)

#define TIMER_HW_BASE                   REALVIEW_TIMER2_3_BASE

static void rt_hw_timer_isr(int vector, void *param)
{
    rt_tick_increase();
    /* clear interrupt */
    TIMER_INTCLR(TIMER_HW_BASE) = 0x01;
}

int rt_hw_timer_init(void)
{
    rt_uint32_t val;

    SYS_CTRL |= REALVIEW_REFCLK;

    /* Setup Timer0 for generating irq */
    val = TIMER_CTRL(TIMER_HW_BASE);
    val &= ~TIMER_CTRL_ENABLE;
    val |= (TIMER_CTRL_32BIT | TIMER_CTRL_PERIODIC | TIMER_CTRL_IE);
    TIMER_CTRL(TIMER_HW_BASE) = val;

    TIMER_LOAD(TIMER_HW_BASE) = 1000;

    /* enable timer */
    TIMER_CTRL(TIMER_HW_BASE) |= TIMER_CTRL_ENABLE;

    rt_hw_interrupt_install(IRQ_PBA8_TIMER2_3, rt_hw_timer_isr, RT_NULL, "tick");
    rt_hw_interrupt_umask(IRQ_PBA8_TIMER2_3);

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
    rt_kprintf("HEAP_BEGIN, %p\n", HEAP_BEGIN);
}

extern void set_secondy_cpu_boot_address(void);
extern void dist_ipi_send(int irq, int cpu);

void secondy_cpu_up(void)
{
    set_secondy_cpu_boot_address();
    __asm__ volatile ("dsb":::"memory");
    dist_ipi_send(0, 1);
}

#include "spinlock.h"
#include "gic.h"
#include "stdint.h"

//#define __REG32(x)                      (*(volatile unsigned int *)(x))

#define REALVIEW_SCTL_BASE              0x10001000  /* System Controller */
#define REALVIEW_REFCLK                 0
#define TIMER01_HW_BASE                 0x10011000
#define TIMER23_HW_BASE                 0x10012000

#define TIMER_LOAD(hw_base)             __REG32(hw_base + 0x00)
#define TIMER_VALUE(hw_base)            __REG32(hw_base + 0x04)
#define TIMER_CTRL(hw_base)             __REG32(hw_base + 0x08)
#define TIMER_CTRL_ONESHOT              (1 << 0)
#define TIMER_CTRL_32BIT                (1 << 1)
#define TIMER_CTRL_DIV1                 (0 << 2)
#define TIMER_CTRL_DIV16                (1 << 2)
#define TIMER_CTRL_DIV256               (2 << 2)
#define TIMER_CTRL_IE                   (1 << 5)        /* Interrupt Enable (versatile only) */
#define TIMER_CTRL_PERIODIC             (1 << 6)
#define TIMER_CTRL_ENABLE               (1 << 7)

#define TIMER_INTCLR(hw_base)           __REG32(hw_base + 0x0c)
#define TIMER_RIS(hw_base)              __REG32(hw_base + 0x10)
#define TIMER_MIS(hw_base)              __REG32(hw_base + 0x14)
#define TIMER_BGLOAD(hw_base)           __REG32(hw_base + 0x18)

#define SYS_CTRL                        __REG32(REALVIEW_SCTL_BASE)

//#define IRQ_PBA8_TIMER2_3               35

void timer_init(int timer, unsigned int preload) {
    uint32_t val;

    if (timer == 0) {
        /* Setup Timer0 for generating irq */
        val = TIMER_CTRL(TIMER01_HW_BASE);
        val &= ~TIMER_CTRL_ENABLE;
        val |= (TIMER_CTRL_32BIT | TIMER_CTRL_PERIODIC | TIMER_CTRL_IE);
        TIMER_CTRL(TIMER01_HW_BASE) = val;

        TIMER_LOAD(TIMER01_HW_BASE) = preload;

        /* enable timer */
        TIMER_CTRL(TIMER01_HW_BASE) |= TIMER_CTRL_ENABLE;
    } else {
        /* Setup Timer1 for generating irq */
        val = TIMER_CTRL(TIMER23_HW_BASE);
        val &= ~TIMER_CTRL_ENABLE;
        val |= (TIMER_CTRL_32BIT | TIMER_CTRL_PERIODIC | TIMER_CTRL_IE);
        TIMER_CTRL(TIMER23_HW_BASE) = val;

        TIMER_LOAD(TIMER23_HW_BASE) = preload;

        /* enable timer */
        TIMER_CTRL(TIMER23_HW_BASE) |= TIMER_CTRL_ENABLE;
    }
}

void timer_clear_pending(int timer) {
    if (timer == 0) {
        TIMER_INTCLR(TIMER01_HW_BASE) = 0x01;
    } else {
        TIMER_INTCLR(TIMER23_HW_BASE) = 0x01;
    }
}

static void rt_hw_timer2_isr(int vector, void *param)
{
    rt_tick_increase();
    /* clear interrupt */
    timer_clear_pending(0);
}

void spin_lock(void);
#if 1
void second_cpu_c_start(void)
{
    rt_hw_vector_init();

    spin_lock();

    arm_gic_cpu_init(0, REALVIEW_GIC_CPU_BASE);
    arm_gic_set_cpu(0, IRQ_PBA8_TIMER0_1, 0x2); //指定到cpu1

    timer_init(0, 1000);
    rt_hw_interrupt_install(IRQ_PBA8_TIMER0_1, rt_hw_timer2_isr, RT_NULL, "tick");
    rt_hw_interrupt_umask(IRQ_PBA8_TIMER0_1);

    rt_system_scheduler_start();
}
#else
void second_cpu_c_start(void)
{
//  rt_hw_vector_init();

    arm_gic_cpu_init(0, REALVIEW_GIC_CPU_BASE);

    __raw_spin_lock(&rt_kernel_lock);

    arm_gic_set_cpu(0, IRQ_PBA8_TIMER0_1, 0x2); //指定到cpu1
    timer_init(0, 2000000);
    rt_hw_interrupt_umask(IRQ_PBA8_TIMER0_1);

    asm volatile ("cpsie i":::"memory","cc");

    __raw_spin_unlock(&rt_kernel_lock);
    rt_kprintf("cpu1 init\n");
	while (1)
    {
//        rt_kprintf(".");
        asm volatile ("wfe");
        __raw_spin_lock(&rt_kernel_lock);
        __raw_spin_unlock(&rt_kernel_lock);
	}
}
#endif

int cnt1 = 0;

#define GIC_ACK_INTID_MASK              0x000003ff
void secondy_irq_handler(void)
{
    int ir;
    int fullir;
    unsigned int cpu_id;

    __asm__ volatile (
            "mrc p15, 0, %0, c0, c0, 5"
            :"=r"(cpu_id)
            );
    cpu_id &= 0xf;

    fullir = arm_gic_get_active_irq(0);
    ir = fullir & GIC_ACK_INTID_MASK;

    if (ir == 1023) {
        /* Spurious interrupt */
        return;
    }
    if (ir < 16) {
        rt_kprintf("IPI %d on cpu %d\n", ir, cpu_id);
    } else if (ir == IRQ_PBA8_TIMER0_1) {
        timer_clear_pending(0);
        rt_kprintf("[cpu%d timer cnt %d]\n", cpu_id, ++cnt1);
    } else {
        rt_kprintf("unkown IRQ no %d 0x%08x\n", ir, cpu_id);
    }

    /* end of interrupt */
    arm_gic_ack(0, fullir);
}
