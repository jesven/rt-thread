#include <rthw.h>
#include <rtthread.h>
#include <stdlib.h>
#include "utest.h"
#include "utest_assert.h"

static rt_hw_spinlock_t print_lock = {0};

static void add_entry0(void* parameter)
{
    while (1)
    {
        int sum = 0;
        int i;

        for (i = 1; i <= 100; i++)
        {
            sum += i;

            rt_enter_critical();
            rt_hw_spin_lock(&print_lock);
            rt_kprintf("cpu%d test0: 1 + ... %d = %d\n", rt_hw_cpu_id(), i, sum);
            rt_hw_spin_unlock(&print_lock);
            rt_exit_critical();
        }
    }
}

static void add_entry1(void* parameter)
{
    while (1)
    {
        int sum = 0;
        int i;

        for (i = 1; i <= 1000; i++)
        {
            sum += i;

            rt_enter_critical();
            rt_hw_spin_lock(&print_lock);
            rt_kprintf("cpu%d test1: 1 + ... %d = %d\n", rt_hw_cpu_id(), i, sum);
            rt_hw_spin_unlock(&print_lock);
            rt_exit_critical();
        }
    }
}

static void add_entry2(void* parameter)
{
    while (1)
    {
        int sum = 0;
        int i;

        for (i = 1; i <= 10000; i++)
        {
            sum += i;

            rt_enter_critical();
            rt_hw_spin_lock(&print_lock);
            rt_kprintf("cpu%d test2: 1 + ... %d = %d\n", rt_hw_cpu_id(), i, sum);
            rt_hw_spin_unlock(&print_lock);
            rt_exit_critical();
        }
    }
}

static void test_smp_create(void* parameter)
{
    rt_thread_t tid;

    tid = rt_thread_create("testadd0", add_entry0, RT_NULL,
        1024, 15, 10);
    if (tid)
    {
        /*
        rt_thread_control(tid, RT_THREAD_CTRL_BIND_CPU, (void*)cpuid);
        */
        rt_thread_startup(tid);
    }

    tid = rt_thread_create("testadd1", add_entry1, RT_NULL,
        1024, 15, 10);
    if (tid)
    {
        /*
        rt_thread_control(tid, RT_THREAD_CTRL_BIND_CPU, (void*)cpuid);
        */
        rt_thread_startup(tid);
    }

    tid = rt_thread_create("testadd2", add_entry2, RT_NULL,
        1024, 15, 10);
    if (tid)
    {
        /*
        rt_thread_control(tid, RT_THREAD_CTRL_BIND_CPU, (void*)cpuid);
        */
        rt_thread_startup(tid);
    }

}

void task(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("test_smp_create", test_smp_create, RT_NULL,
        1024, 10, 10);
    if (tid)
    {
        /*
        rt_thread_control(tid, RT_THREAD_CTRL_BIND_CPU, (void*)cpuid);
        */
        rt_thread_startup(tid);
    }
}

static rt_err_t utest_tc_init(void)
{
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(task);
}
UTEST_TC_EXPORT(testcase, "s1to100.test", utest_tc_init, utest_tc_cleanup, 10);
