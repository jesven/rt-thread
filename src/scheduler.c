/*
 * File      : scheduler.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-03-17     Bernard      the first version
 * 2006-04-28     Bernard      fix the scheduler algorthm
 * 2006-04-30     Bernard      add SCHEDULER_DEBUG
 * 2006-05-27     Bernard      fix the scheduler algorthm for same priority
 *                             thread schedule
 * 2006-06-04     Bernard      rewrite the scheduler algorithm
 * 2006-08-03     Bernard      add hook support
 * 2006-09-05     Bernard      add 32 priority level support
 * 2006-09-24     Bernard      add rt_system_scheduler_start function
 * 2009-09-16     Bernard      fix _rt_scheduler_stack_check
 * 2010-04-11     yi.qiu       add module feature
 * 2010-07-13     Bernard      fix the maximal number of rt_scheduler_lock_nest
 *                             issue found by kuronca
 * 2010-12-13     Bernard      add defunct list initialization even if not use heap.
 * 2011-05-10     Bernard      clean scheduler debug log.
 * 2013-12-21     Grissiom     add rt_critical_level
 */

#include <rtthread.h>
#include <rthw.h>
#include "spinlock.h"

rt_list_t rt_global_thread_priority_table[RT_THREAD_PRIORITY_MAX];
rt_list_t rt_percpu_thread_priority_table[RT_CPUS_NR][RT_THREAD_PRIORITY_MAX];

struct rt_thread *rt_percpu_current_thread[RT_CPUS_NR];

rt_uint8_t rt_percpu_current_priority[RT_CPUS_NR];

#if RT_THREAD_PRIORITY_MAX > 32
/* Maximum priority level, 256 */
rt_uint32_t rt_global_thread_ready_priority_group;
rt_uint8_t rt_global_thread_ready_table[32];
rt_uint32_t rt_percpu_thread_ready_priority_group[RT_CPUS_NR];
rt_uint8_t rt_percpu_thread_ready_table[RT_CPUS_NR][32];
#else
/* Maximum priority level, 32 */
rt_uint32_t rt_global_thread_ready_priority_group;
rt_uint32_t rt_percpu_thread_ready_priority_group[RT_CPUS_NR];
#endif

rt_list_t rt_thread_defunct;

#ifdef RT_USING_HOOK
static void (*rt_scheduler_hook)(struct rt_thread *from, struct rt_thread *to);

/**
 * @addtogroup Hook
 */

/**@{*/

/**
 * This function will set a hook function, which will be invoked when thread
 * switch happens.
 *
 * @param hook the hook function
 */
void
rt_scheduler_sethook(void (*hook)(struct rt_thread *from, struct rt_thread *to))
{
    rt_scheduler_hook = hook;
}

/**@}*/
#endif

#ifdef RT_USING_OVERFLOW_CHECK
static void _rt_scheduler_stack_check(struct rt_thread *thread)
{
    RT_ASSERT(thread != RT_NULL);

    if (*((rt_uint8_t *)thread->stack_addr) != '#' ||
        (rt_uint32_t)thread->sp <= (rt_uint32_t)thread->stack_addr ||
        (rt_uint32_t)thread->sp >
        (rt_uint32_t)thread->stack_addr + (rt_uint32_t)thread->stack_size)
    {
        rt_uint32_t level;

        rt_kprintf("thread:%s stack overflow\n", thread->name);
#ifdef RT_USING_FINSH
        {
            extern long list_thread(void);
            list_thread();
        }
#endif
        level = rt_hw_interrupt_disable();
        while (level);
    }
    else if ((rt_uint32_t)thread->sp <= ((rt_uint32_t)thread->stack_addr + 32))
    {
        rt_kprintf("warning: %s stack is close to end of stack address.\n",
                   thread->name);
    }
}
#endif

/**
 * @ingroup SystemInit
 * This function will initialize the system scheduler
 */
void rt_system_scheduler_init(void)
{
    register rt_base_t offset;
    int cpu;

    RT_DEBUG_LOG(RT_DEBUG_SCHEDULER, ("start scheduler: max priority 0x%02x\n",
                                      RT_THREAD_PRIORITY_MAX));

    for (offset = 0; offset < RT_THREAD_PRIORITY_MAX; offset ++)
    {
        rt_list_init(&rt_global_thread_priority_table[offset]);
    }
    for (cpu = 0; cpu < RT_CPUS_NR; cpu++)
    {
        for (offset = 0; offset < RT_THREAD_PRIORITY_MAX; offset ++)
        {
            rt_list_init(&rt_percpu_thread_priority_table[cpu][offset]);
        }

        rt_percpu_current_priority[cpu] = RT_THREAD_PRIORITY_MAX - 1;
        rt_percpu_current_thread[cpu] = RT_NULL;
    }

    /* initialize ready priority group */
    rt_global_thread_ready_priority_group = 0;

    rt_memset(rt_percpu_thread_ready_priority_group, 0, sizeof(rt_percpu_thread_ready_priority_group));

#if RT_THREAD_PRIORITY_MAX > 32
    /* initialize ready table */
    rt_memset(rt_global_thread_ready_table, 0, sizeof(rt_global_thread_ready_table));
    rt_memset(rt_percpu_thread_ready_table, 0, sizeof(rt_percpu_thread_ready_table));
#endif

    /* initialize thread defunct */
    rt_list_init(&rt_thread_defunct);
}

/**
 * @ingroup SystemInit
 * This function will startup scheduler. It will select one thread
 * with the highest priority level, then switch to it.
 */
void rt_system_scheduler_start(void)
{
    register struct rt_thread *to_thread;
    register rt_ubase_t highest_ready_priority, l_highest_ready_priority;

#if RT_THREAD_PRIORITY_MAX > 32
    register rt_ubase_t number;

    number = __rt_ffs(rt_global_thread_ready_priority_group) - 1;
    highest_ready_priority = (number << 3) + __rt_ffs(rt_global_thread_ready_table[number]) - 1;
#else
    highest_ready_priority = __rt_ffs(rt_global_thread_ready_priority_group) - 1;
#endif

#if RT_THREAD_PRIORITY_MAX > 32
    number = __rt_ffs(rt_thread_ready_priority_group) - 1;
    l_highest_ready_priority = (number << 3) + __rt_ffs(rt_thread_ready_table[number]) - 1;
#else
    l_highest_ready_priority = __rt_ffs(rt_thread_ready_priority_group) - 1;
#endif

    if (highest_ready_priority <= l_highest_ready_priority) //这里用=,表示全局任务更优先
    {
        /* get switch to thread */
        to_thread = rt_list_entry(rt_global_thread_priority_table[highest_ready_priority].next,
                                  struct rt_thread,
                                  tlist);
    }
    else
    {
        /* get switch to thread */
        to_thread = rt_list_entry(rt_thread_priority_table[l_highest_ready_priority].next,
                                  struct rt_thread,
                                  tlist);
    }

    //rt_current_thread = to_thread;

    to_thread->oncpu = 1;
    rt_schedule_remove_thread(to_thread);

    /* switch to new thread */
    rt_hw_context_switch_to((rt_uint32_t)&to_thread->sp, to_thread);

    /* never come back */
}

/**
 * @addtogroup Thread
 */

/**@{*/

/**
 * This function will perform one schedule. It will select one thread
 * with the highest priority level, then switch to it.
 */
void rt_schedule(void)
{
    rt_base_t level;
    struct rt_thread *to_thread;
    struct rt_thread *from_thread;

    /* disable interrupt */
    level = rt_hw_interrupt_disable();

    /* check the scheduler is enabled or not */

    if (rt_current_thread->scheduler_lock_nest == 1 && rt_interrupt_nest == 0)
    {
        register rt_ubase_t highest_ready_priority, l_highest_ready_priority;

        if (rt_global_thread_ready_priority_group != 0 || rt_thread_ready_priority_group != 0)
        {
            rt_current_thread->oncpu = 0;
            if ((rt_current_thread->stat & RT_THREAD_STAT_MASK) == RT_THREAD_READY)
            {
                rt_schedule_insert_thread(rt_current_thread);
            }

#if RT_THREAD_PRIORITY_MAX <= 32
            highest_ready_priority = __rt_ffs(rt_global_thread_ready_priority_group) - 1;
#else
            register rt_ubase_t number;

            number = __rt_ffs(rt_global_thread_ready_priority_group) - 1;
            highest_ready_priority = (number << 3) + __rt_ffs(rt_global_thread_ready_table[number]) - 1;
#endif

#if RT_THREAD_PRIORITY_MAX <= 32
            l_highest_ready_priority = __rt_ffs(rt_thread_ready_priority_group) - 1;
#else
            number = __rt_ffs(rt_thread_ready_priority_group) - 1;
            l_highest_ready_priority = (number << 3) + __rt_ffs(rt_thread_ready_table[number]) - 1;
#endif

            if (highest_ready_priority <= l_highest_ready_priority)
            {
                /* get switch to thread */
                to_thread = rt_list_entry(rt_global_thread_priority_table[highest_ready_priority].next,
                        struct rt_thread,
                        tlist);
            }
            else
            {
                /* get switch to thread */
                to_thread = rt_list_entry(rt_thread_priority_table[l_highest_ready_priority].next,
                        struct rt_thread,
                        tlist);
                highest_ready_priority = l_highest_ready_priority;
            }

            if (to_thread != rt_current_thread)
            {
                /* if the destination thread is not the same as current thread */
                rt_current_priority = (rt_uint8_t)highest_ready_priority;
                from_thread         = rt_current_thread;

                RT_OBJECT_HOOK_CALL(rt_scheduler_hook, (from_thread, to_thread));

                to_thread->oncpu = 1;
                rt_schedule_remove_thread(to_thread);

                /* switch to new thread */
                RT_DEBUG_LOG(RT_DEBUG_SCHEDULER,
                        ("[%d]switch to priority#%d "
                         "thread:%.*s(sp:0x%p), "
                         "from thread:%.*s(sp: 0x%p)\n",
                         rt_interrupt_nest, highest_ready_priority,
                         RT_NAME_MAX, to_thread->name, to_thread->sp,
                         RT_NAME_MAX, from_thread->name, from_thread->sp));

#ifdef RT_USING_OVERFLOW_CHECK
                _rt_scheduler_stack_check(to_thread);
#endif

                {
                    extern void rt_thread_handle_sig(rt_bool_t clean_state);

                    rt_hw_context_switch((rt_uint32_t)&from_thread->sp,
                            (rt_uint32_t)&to_thread->sp, to_thread);

                    /* enable interrupt */
                    rt_hw_interrupt_enable(level);

#ifdef RT_USING_SIGNALS
                    /* check signal status */
                    rt_thread_handle_sig(RT_TRUE);
#endif
                }
            }
            else
            {
                rt_current_thread->oncpu = 1;
                rt_schedule_remove_thread(rt_current_thread);
                /* enable interrupt */
                rt_hw_interrupt_enable(level);
            }
        }
        else
        {
            /* enable interrupt */
            rt_hw_interrupt_enable(level);
        }
    }
    else
    {
        /* enable interrupt */
        rt_hw_interrupt_enable(level);
    }
}

void rt_interrupt_check_schedule(void)
{
    struct rt_thread *to_thread;
    struct rt_thread *from_thread;

    if (rt_current_thread->scheduler_lock_nest == 1 && rt_interrupt_nest == 0)
    {
        register rt_ubase_t highest_ready_priority, l_highest_ready_priority;

        if (rt_global_thread_ready_priority_group != 0 || rt_thread_ready_priority_group != 0)
        {
            rt_current_thread->oncpu = 0;
            if ((rt_current_thread->stat & RT_THREAD_STAT_MASK) == RT_THREAD_READY)
            {
                rt_schedule_insert_thread(rt_current_thread);
            }
#if RT_THREAD_PRIORITY_MAX <= 32
            highest_ready_priority = __rt_ffs(rt_global_thread_ready_priority_group) - 1;
#else
            register rt_ubase_t number;

            number = __rt_ffs(rt_global_thread_ready_priority_group) - 1;
            highest_ready_priority = (number << 3) + __rt_ffs(rt_global_thread_ready_table[number]) - 1;
#endif

#if RT_THREAD_PRIORITY_MAX <= 32
            l_highest_ready_priority = __rt_ffs(rt_thread_ready_priority_group) - 1;
#else
            number = __rt_ffs(rt_thread_ready_priority_group) - 1;
            l_highest_ready_priority = (number << 3) + __rt_ffs(rt_thread_ready_table[number]) - 1;
#endif

            if (highest_ready_priority <= l_highest_ready_priority)
            {
                /* get switch to thread */
                to_thread = rt_list_entry(rt_global_thread_priority_table[highest_ready_priority].next,
                        struct rt_thread,
                        tlist);
            }
            else
            {
                /* get switch to thread */
                to_thread = rt_list_entry(rt_thread_priority_table[l_highest_ready_priority].next,
                        struct rt_thread,
                        tlist);
                highest_ready_priority = l_highest_ready_priority;
            }

            if (to_thread != rt_current_thread)
            {
                /* if the destination thread is not the same as current thread */
                rt_current_priority = (rt_uint8_t)highest_ready_priority;
                from_thread         = rt_current_thread;

                RT_OBJECT_HOOK_CALL(rt_scheduler_hook, (from_thread, to_thread));
                to_thread->oncpu = 1;
                rt_schedule_remove_thread(to_thread);

#ifdef RT_USING_OVERFLOW_CHECK
                _rt_scheduler_stack_check(to_thread);
#endif
                RT_DEBUG_LOG(RT_DEBUG_SCHEDULER, ("switch in interrupt\n"));

                rt_hw_context_switch_interrupt((rt_uint32_t)&from_thread->sp,
                        (rt_uint32_t)&to_thread->sp, to_thread);
            }
            else
            {
                rt_current_thread->oncpu = 1;
                rt_schedule_remove_thread(rt_current_thread);
            }
        }
    }
}

/*
 * This function will insert a thread to system ready queue. The state of
 * thread will be set as READY and remove from suspend queue.
 *
 * @param thread the thread to be inserted
 * @note Please do not invoke this function in user application.
 */
void rt_schedule_insert_thread(struct rt_thread *thread)
{
    register rt_base_t temp;

    RT_ASSERT(thread != RT_NULL);

    /* disable interrupt */
    temp = rt_hw_interrupt_disable();

    /* change stat */
    thread->stat = RT_THREAD_READY | (thread->stat & ~RT_THREAD_STAT_MASK);

    if (thread->oncpu)
    {
        rt_hw_interrupt_enable(temp);
        return;
    }

    /* insert thread to ready list */
    if (thread->bind_cpu == RT_CPUS_NR)
    {
        rt_list_insert_before(&(rt_global_thread_priority_table[thread->current_priority]),
                              &(thread->tlist));
    }
    else
    {
        rt_list_insert_before(&(rt_percpu_thread_priority_table[thread->bind_cpu][thread->current_priority]),
                              &(thread->tlist));
    }

    /* set priority mask */
#if RT_THREAD_PRIORITY_MAX <= 32
    RT_DEBUG_LOG(RT_DEBUG_SCHEDULER, ("insert thread[%.*s], the priority: %d\n",
                                      RT_NAME_MAX, thread->name, thread->current_priority));
#else
    RT_DEBUG_LOG(RT_DEBUG_SCHEDULER,
                 ("insert thread[%.*s], the priority: %d 0x%x %d\n",
                  RT_NAME_MAX,
                  thread->name,
                  thread->number,
                  thread->number_mask,
                  thread->high_mask));
#endif

    if (thread->bind_cpu == RT_CPUS_NR)
    {
#if RT_THREAD_PRIORITY_MAX > 32
        rt_global_thread_ready_table[thread->number] |= thread->high_mask;
#endif
        rt_global_thread_ready_priority_group |= thread->number_mask;
    }
    else
    {
#if RT_THREAD_PRIORITY_MAX > 32
        rt_percpu_thread_ready_table[thread->bind_cpu][thread->number] |= thread->high_mask;
#endif
        rt_percpu_thread_ready_priority_group[thread->bind_cpu] |= thread->number_mask;
    }

    /* enable interrupt */
    rt_hw_interrupt_enable(temp);
}

/*
 * This function will remove a thread from system ready queue.
 *
 * @param thread the thread to be removed
 *
 * @note Please do not invoke this function in user application.
 */
void rt_schedule_remove_thread(struct rt_thread *thread)
{
    register rt_base_t temp;

    RT_ASSERT(thread != RT_NULL);

    /* disable interrupt */
    temp = rt_hw_interrupt_disable();

#if RT_THREAD_PRIORITY_MAX <= 32
    RT_DEBUG_LOG(RT_DEBUG_SCHEDULER, ("remove thread[%.*s], the priority: %d\n",
                                      RT_NAME_MAX, thread->name,
                                      thread->current_priority));
#else
    RT_DEBUG_LOG(RT_DEBUG_SCHEDULER,
                 ("remove thread[%.*s], the priority: %d 0x%x %d\n",
                  RT_NAME_MAX,
                  thread->name,
                  thread->number,
                  thread->number_mask,
                  thread->high_mask));
#endif

    /* remove thread from ready list */
    rt_list_remove(&(thread->tlist));
    if (thread->bind_cpu == RT_CPUS_NR)
    {
        if (rt_list_isempty(&(rt_global_thread_priority_table[thread->current_priority])))
        {
#if RT_THREAD_PRIORITY_MAX > 32
            rt_global_thread_ready_table[thread->number] &= ~thread->high_mask;
            if (rt_global_thread_ready_table[thread->number] == 0)
            {
                rt_global_thread_ready_priority_group &= ~thread->number_mask;
            }
#else
            rt_global_thread_ready_priority_group &= ~thread->number_mask;
#endif
        }
    }
    else
    {
        if (rt_list_isempty(&(rt_percpu_thread_priority_table[thread->bind_cpu][thread->current_priority])))
        {
#if RT_THREAD_PRIORITY_MAX > 32
            rt_percpu_thread_ready_table[thread->bind_cpu][thread->number] &= ~thread->high_mask;
            if (rt_global_thread_ready_table[thread->number] == 0)
            {
                rt_percpu_thread_ready_priority_group[thread->bind_cpu] &= ~thread->number_mask;
            }
#else
            rt_percpu_thread_ready_priority_group[thread->bind_cpu] &= ~thread->number_mask;
#endif
        }
    }

    /* enable interrupt */
    rt_hw_interrupt_enable(temp);
}

/**
 * This function will lock the thread scheduler.
 */

raw_spinlock_t rt_scheduler_lock = {.slock = 0};

void rt_enter_critical(void)
{
    register rt_base_t level;

    /* disable interrupt */
    level = rt_disable_local_irq();

    /*
     * the maximal number of nest is RT_UINT16_MAX, which is big
     * enough and does not check here
     */

    if (rt_current_thread->scheduler_lock_nest == rt_current_thread->kernel_lock_nest)
    {
        __raw_spin_lock(&rt_scheduler_lock);
    }
    rt_current_thread->scheduler_lock_nest ++;

    /* enable interrupt */
    rt_enable_local_irq(level);
}
RTM_EXPORT(rt_enter_critical);

/**
 * This function will unlock the thread scheduler.
 */
void rt_exit_critical(void)
{
    register rt_base_t level;

    /* disable interrupt */
    level = rt_disable_local_irq();

    rt_current_thread->scheduler_lock_nest --;

    if (rt_current_thread->scheduler_lock_nest == rt_current_thread->kernel_lock_nest)
    {
        __raw_spin_unlock(&rt_scheduler_lock);
    }

    if (rt_current_thread->scheduler_lock_nest <= 0)
    {
        rt_current_thread->scheduler_lock_nest = 0;
        /* enable interrupt */
        rt_enable_local_irq(level);

        rt_schedule();
    }
    else
    {
        /* enable interrupt */
        rt_enable_local_irq(level);
    }
}
RTM_EXPORT(rt_exit_critical);

/**
 * Get the scheduler lock level
 *
 * @return the level of the scheduler lock. 0 means unlocked.
 */
rt_uint16_t rt_critical_level(void)
{
    return rt_current_thread->scheduler_lock_nest;
}
RTM_EXPORT(rt_critical_level);
/**@}*/

void rt_post_switch(struct rt_thread *thread)
{
#if 0
    rt_kprintf("%d S %s -> %s\n", rt_cpuid(), rt_current_thread->name, thread->name);
#endif
    rt_current_thread = thread;
    if (!thread->kernel_lock_nest)
    {
        spin_unlock();
    }
}
RTM_EXPORT(rt_post_switch);

void rt_post_switch_int(struct rt_thread *thread)
{
#if 0
    rt_kprintf("%d I %s -> %s\n", rt_cpuid(), rt_current_thread->name, thread->name);
#endif
    rt_current_thread->kernel_lock_nest--;
    rt_current_thread->scheduler_lock_nest--;
    rt_current_thread = thread;
    if (!thread->kernel_lock_nest)
    {
        spin_unlock();
    }
}
RTM_EXPORT(rt_post_switch_int);
