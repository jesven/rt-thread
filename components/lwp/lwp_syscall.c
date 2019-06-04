/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-06-10     Bernard      first version
 */

/* RT-Thread System call */
#include <lwp.h>
#include <lwp_mem.h>
#include <lwp_syscall.h>

#include <dfs_poll.h>
#include <dfs_select.h>

#if (defined(RT_USING_SAL) && defined(SAL_USING_POSIX))
#include <sys/socket.h>

#define SYSCALL_NET(f) ((void*)(f))
#else
#define SYSCALL_NET(f) ((void*)sys_notimpl)
#endif

#define DBG_TAG    "LWP_CALL"
#define DBG_LVL           DBG_WARNING
#include <rtdbg.h>

static void __exit_files(rt_thread_t tid)
{
    struct rt_lwp *lwp;

    lwp = (struct rt_lwp *)tid->lwp;
    while (lwp->fdt.maxfd > 0)
    {
        lwp->fdt.maxfd --;
        close(lwp->fdt.maxfd);
    }
}

/* thread/process */
void sys_exit(int value)
{
    rt_thread_t tid;

    /* TODO: handle the return_value */
    dbg_log(DBG_LOG, "enter sys_exit\n");
    tid = rt_thread_self();
    __exit_files(tid);
    rt_thread_delete(tid);

    rt_schedule();

    return;
}

/* syscall: "read" ret: "ssize_t" args: "int" "void *" "size_t" */
ssize_t sys_read(int fd, void *buf, size_t nbyte)
{
    return read(fd, buf, nbyte);
}

/* syscall: "write" ret: "ssize_t" args: "int" "const void *" "size_t" */
ssize_t sys_write(int fd, const void *buf, size_t nbyte)
{
    return write(fd, buf, nbyte);
}

/* syscall: "lseek" ret: "off_t" args: "int" "off_t" "int" */
off_t sys_lseek(int fd, off_t offset, int whence)
{
    return lseek(fd, offset, whence);
}

/* syscall: "open" ret: "int" args: "const char *" "int" "..." */
int sys_open(const char *name, int mode, ...)
{
    return open(name, mode, 0);
}

/* syscall: "close" ret: "int" args: "int" */
int sys_close(int fd)
{
    return close(fd);
}

/* syscall: "ioctl" ret: "int" args: "int" "u_long" "..." */
int sys_ioctl(int fd, unsigned long cmd, void* data)
{
    return ioctl(fd, cmd, data);
}

/* syscall: "nanosleep" ret: "int" args: "const struct timespec *" "struct timespec *" */
int sys_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    rt_tick_t tick;

    dbg_log(DBG_LOG, "sys_nanosleep\n");

    tick = rqtp->tv_sec * RT_TICK_PER_SECOND + (rqtp->tv_nsec * RT_TICK_PER_SECOND)/ 1000000000;
    rt_thread_delay(tick);

    if (rmtp)
    {
        tick = rt_tick_get() - tick;
        /* get the passed time */
        rmtp->tv_sec = tick/RT_TICK_PER_SECOND;
        rmtp->tv_nsec = (tick%RT_TICK_PER_SECOND) * (1000000000/RT_TICK_PER_SECOND);
    }

    return 0;
}

/* syscall: "getpriority" ret: "int" args: "int" "id_t" */
int sys_getpriority(int which, id_t who)
{
    if (which == PRIO_PROCESS)
    {
        rt_thread_t tid;

        tid = rt_thread_self();
        if (who == (id_t)tid || who == 0xff)
        {
            return tid->current_priority;
        }
    }

    return 0xff;
}

/* syscall: "setpriority" ret: "int" args: "int" "id_t" "int" */
int sys_setpriority(int which, id_t who, int prio)
{
    if (which == PRIO_PROCESS)
    {
        rt_thread_t tid;

        tid = rt_thread_self();
        if ((who == (id_t)tid || who == 0xff) && (prio >= 0 && prio < RT_THREAD_PRIORITY_MAX))
        {
            rt_thread_control(tid, RT_THREAD_CTRL_CHANGE_PRIORITY, &prio);
            return 0;
        }
    }

    return -1;
}

/* syscall: "gettimeofday" ret: "int" args: "struct timeval *" "struct timezone *" */
int sys_gettimeofday(struct timeval *tp, struct timezone *tzp)
{
    if (tp)
    {
        tp->tv_sec = rt_tick_get() / RT_TICK_PER_SECOND;
        tp->tv_usec = (rt_tick_get() % RT_TICK_PER_SECOND) * (1000000 / RT_TICK_PER_SECOND);
    }

    return 0;
}

/* syscall: "settimeofday" ret: "int" args: "const struct timeval *" "const struct timezone *" */
int sys_settimeofday(const struct timeval *tv, const struct timezone *tzp)
{
    return 0;
}

/* syscall: "msgget" ret: "int" args: "key_t" "int" */
int sys_msgget(key_t key, int msgflg)
{
    return -1;
}

/* syscall: "msgsnd" ret: "int" args: "int" "const void *" "size_t" "int" */
int sys_msgsend(int msqid, const void *msgp, size_t msgsz, int msgflg)
{
    return -1;
}

/* syscall: "msgrcv" ret: "int" args: "int" "void *" "size_t" "long" "int" */
int sys_msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
    return -1;
}

/* syscall: "sys_log" ret: "int" args: "const char*" "size" */
int sys_log(const char* log, int size)
{
    rt_device_t console = rt_console_get_device();

    if (console) rt_device_write(console, -1, log, size);

    return 0;
}

void *sys_malloc(size_t size)
{
    return rt_lwp_mem_malloc(size);
}

void sys_free(void *addr)
{
    rt_lwp_mem_free(addr);
}

void *sys_realloc(void *rmem, size_t newsize)
{
    return rt_lwp_mem_realloc(rmem, newsize);
}

int sys_fstat(int file, struct stat *buf)
{
    return fstat(file, buf);
}

int sys_notimpl(void)
{
    return -ENOSYS;
}

#ifdef RT_USING_ENDPOINT
/* ipc */

void * sys_endpoint_create(const char *name)
{
    return (void*)rt_endpoint_create(name);
}

int sys_endpoint_delete(void *ep)
{
    return (int)rt_endpoint_delete((rt_endpoint_t)ep);
}

void * sys_endpoint_find(const char *name)
{
    return (void*)rt_endpoint_find(name);
}

int sys_ipc_send(void* ep, void *data, uint8_t need_reply, void* *data_ret)
{
    rt_ipc_msg_t msg;
    rt_ipc_msg_t msg_ret = RT_NULL;
    int ret;

    msg = (rt_ipc_msg_t)rt_malloc(sizeof *msg);
    if (!msg)
    {
        return -ENOMEM;
    }
    rt_ipc_msg_init(msg, data, need_reply);
    ret = (int)rt_ipc_send((rt_endpoint_t)ep, msg, &msg_ret);
    if (need_reply && msg_ret)
    {
        *data_ret = msg_ret->data;
        rt_free(msg_ret);
    }
    return ret;
}

int sys_ipc_reply(void* ep, void* data)
{
    rt_ipc_msg_t msg;
    int ret;

    msg = (rt_ipc_msg_t)rt_malloc(sizeof *msg);
    if (!msg)
    {
        return -ENOMEM;
    }
    rt_ipc_msg_init(msg, data, 0);
    ret = (int)rt_ipc_reply((rt_endpoint_t)ep, msg);
    return ret;
}

int sys_ipc_recv(void* ep, void* *data)
{
    rt_ipc_msg_t msg = 0;
    int ret;

    ret = (int)rt_ipc_recv((rt_endpoint_t)ep, &msg);
    if (msg)
    {
        *data = msg->data;
        rt_free(msg);
    }
    return ret;
}
#endif

const static void* func_table[] =
{
    (void *)sys_exit,           // 0x01
    (void *)sys_read,           // 0x02
    (void *)sys_write,          // 0x03
    (void *)sys_lseek,          // 0x04
    (void *)sys_open,           // 0x05
    (void *)sys_close,          // 0x06
    (void *)sys_ioctl,          // 0x07

    (void *)sys_nanosleep,      // 0x08

    (void *)sys_getpriority,    // 0x09
    (void *)sys_setpriority,    // 0x0a

    (void *)sys_gettimeofday,   // 0x0b
    (void *)sys_settimeofday,   // 0x0c

    (void *)sys_malloc,         // 0x0d
    (void *)sys_free,           // 0x0e
    (void *)sys_realloc,        //0x0f
    (void *)sys_fstat,          // 0x10
    (void *)poll,               // 0x11

    SYSCALL_NET(accept),     // 0x12
    SYSCALL_NET(bind),       // 0x13
    SYSCALL_NET(shutdown),   // 0x14
    SYSCALL_NET(getpeername),// 0x15
    SYSCALL_NET(getsockname),// 0x16
    SYSCALL_NET(getsockopt), // 0x17
    SYSCALL_NET(setsockopt), // 0x18
    SYSCALL_NET(connect),    // 0x19
    SYSCALL_NET(listen),     // 0x1a
    SYSCALL_NET(recv),       // 0x1b
    SYSCALL_NET(recvfrom),   // 0x1c
    SYSCALL_NET(send),       // 0x1d
    SYSCALL_NET(sendto),     // 0x1e
    SYSCALL_NET(socket),     // 0x1f

    (void *)select,          // 0x20

#ifdef RT_USING_ENDPOINT
    /* ipc */
    (void *)sys_endpoint_create, // 0x21
    (void *)sys_endpoint_delete, // 0x22
    (void *)sys_endpoint_find,   // 0x23
    (void *)sys_ipc_send,        // 0x24
    (void *)sys_ipc_reply,       // 0x25
    (void *)sys_ipc_recv,        // 0x26
#endif
};

const void *lwp_get_sys_api(rt_uint32_t number)
{
    const void *func = (const void*)sys_notimpl;

    if (number == 0xff)
    {
        func = (void *)sys_log;
    }
    else
    {
        number -= 1;
        if (number < sizeof(func_table)/sizeof(func_table[0]))
        {
            func = func_table[number];
        }
    }

    return func;
}
