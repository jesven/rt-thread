#include "rtthread.h"
#include "rthw.h"
#include "spinlock.h"

int _kernel_lock[RT_CPUS_NR] = {0};
#define kernel_lock _kernel_lock[rt_cpuid()]

raw_spinlock_t rt_kernel_lock;

void spin_lock(void)
{
    if (kernel_lock == 0)
    {
        __raw_spin_lock(&rt_kernel_lock);
        kernel_lock = 1;
    }
    else
    {
        rt_kprintf("bug %d\n", rt_current_thread->kernel_lock_nest);
        while(1);
    }
}

void spin_unlock(void)
{
    if (kernel_lock)
    {
        kernel_lock = 0;
        __raw_spin_unlock(&rt_kernel_lock);
    }
}

rt_base_t rt_hw_interrupt_disable(void)
{
    rt_base_t level;
    level = rt_disable_local_irq();
    if (rt_current_thread != RT_NULL)
    {
        if (rt_current_thread->kernel_lock_nest++ == 0)
        {
            rt_current_thread->scheduler_lock_nest++;
            spin_lock();
        }
    }
    return level;
}

void rt_hw_interrupt_enable(rt_base_t level)
{
    if (rt_current_thread != RT_NULL)
    {
        if (--rt_current_thread->kernel_lock_nest == 0)
        {
            rt_current_thread->scheduler_lock_nest--;
            spin_unlock();
        }
    }
    rt_enable_local_irq(level);
}

void rt_hw_interrupt_disable_int(void)
{
    if (rt_current_thread != RT_NULL)
    {
        rt_current_thread->kernel_lock_nest++;
        rt_current_thread->scheduler_lock_nest++;
        spin_lock();
    }
}

void rt_hw_interrupt_enable_int(void)
{
    if (rt_current_thread != RT_NULL)
    {
        spin_unlock();
        rt_current_thread->kernel_lock_nest--;
        rt_current_thread->scheduler_lock_nest--;
    }
}
