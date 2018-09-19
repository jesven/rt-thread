#include "rtthread.h"
#include "rthw.h"

extern struct rt_thread *rt_current_thread;

int kernel_lock = 0;
void spin_lock(void)
{
    if (kernel_lock == 0)
    {
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
    }
}

rt_base_t rt_hw_interrupt_disable(void)
{
    rt_base_t level;
    if (rt_current_thread != RT_NULL)
    {
        if (rt_current_thread->kernel_lock_nest++ == 0)
        {
            level = rt_disable_local_irq();
            rt_current_thread->scheduler_lock_nest++;
            spin_lock();
        }
        else
            level = rt_disable_local_irq();
    }
    else
        level = rt_disable_local_irq();
    return level;
}

void rt_hw_interrupt_enable(rt_base_t level)
{
    if (rt_current_thread != RT_NULL)
    {
        if (rt_current_thread->kernel_lock_nest > 0)
        {
            rt_current_thread->kernel_lock_nest--;
            if (rt_current_thread->kernel_lock_nest == 0)
            {
                spin_unlock();
                rt_current_thread->scheduler_lock_nest--;
            }
        }
    }
    rt_enable_local_irq(level);
}
