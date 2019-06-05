/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-03-12     Bernard      first version
 * 2018-11-02     heyuanjie    fix complie error in iar
 */

#include <rtthread.h>
#include <rthw.h>
#include <dfs_posix.h>
#include <elf.h>

#ifndef RT_USING_DFS
    #error  "lwp need file system(RT_USING_DFS)"
#endif

#include "lwp.h"

#define DBG_TAG    "LWP"
#define DBG_LVL    DBG_WARNING
#include <rtdbg.h>

extern void lwp_user_entry(void *args, const void *text, void *data);

/**
 * RT-Thread light-weight process
 */
void lwp_set_kernel_sp(uint32_t *sp)
{
    struct rt_lwp *user_data;
    user_data = (struct rt_lwp *)rt_thread_self()->lwp;
    user_data->kernel_sp = sp;
}

uint32_t *lwp_get_kernel_sp(void)
{
    struct rt_lwp *user_data;
    user_data = (struct rt_lwp *)rt_thread_self()->lwp;

    return user_data->kernel_sp;
}

static int lwp_argscopy(struct rt_lwp *lwp, int argc, char **argv)
{
    int size = sizeof(int)*3; /* store argc, argv, NULL */
    int *args;
    char *str;
    char **new_argv;
    int i;
    int len;

    for (i = 0; i < argc; i ++)
    {
        size += (rt_strlen(argv[i]) + 1);
    }
    size  += (sizeof(int) * argc);

    args = (int*)rt_malloc(size);
    if (args == RT_NULL)
        return -1;

    str = (char*)((int)args + (argc + 3) * sizeof(int));
    new_argv = (char**)&args[2];
    args[0] = argc;
    args[1] = (int)new_argv;

    for (i = 0; i < argc; i ++)
    {
        len = rt_strlen(argv[i]) + 1;
        new_argv[i] = str;
        rt_memcpy(str, argv[i], len);
        str += len;
    }
    new_argv[i] = 0;
    lwp->args = args;

    return 0;
}

#define dprintf(...)

static const char elf_magic[] = {0x7f, 'E', 'L', 'F'};

#define check_off(voff, vlen) do {\
                                if (voff > vlen) { \
                                    result = -RT_ERROR; \
                                    goto _exit;\
                                }\
                            } while (0)

#define check_read(vrlen, vrlen_want) do {\
                                if (vrlen < vrlen_want) { \
                                    result = -RT_ERROR; \
                                    goto _exit;\
                                }\
                            } while (0)

static size_t load_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t read_block = 0;

    while (nmemb) {
        size_t rc;

        if (feof(stream)) {
            rt_kprintf("ERROR: file size error\n");
            break;
        }
        rc = fread(ptr, size, nmemb, stream);
        ptr += rc * size;
        nmemb -= rc;
        read_block += rc;
    }
    return read_block;
}

static int load_elf(FILE *fp, int len, struct rt_lwp *lwp, uint8_t *load_addr)
{
    uint32_t i;
    uint32_t off = 0;
    char *p_section_str = 0;
    Elf32_Ehdr eheader;
    Elf32_Phdr pheader;
    Elf32_Shdr sheader;
    int result = RT_EOK;
    uint32_t text_vaddr;
    uint32_t magic;
    size_t read_len;

    if (len < sizeof eheader)
    {
        return -RT_ERROR;
    }

    fseek(fp, 0, SEEK_SET);
    read_len = load_fread(&magic, 1, sizeof magic, fp);
    check_read(read_len, sizeof magic);

    if (memcmp(elf_magic, &magic, 4) != 0)
    {
        return -RT_ERROR;
    }

    fseek(fp, off, SEEK_SET);
    read_len = load_fread(&eheader, 1, sizeof eheader, fp);
    check_read(read_len, sizeof eheader);

    if (eheader.e_ident[4] != 1)
    { /* not 32bit */
        return -RT_ERROR;
    }
    if (eheader.e_ident[6] != 1)
    { /* ver not 1 */
        return -RT_ERROR;
    }
    if (eheader.e_type != 2)
    { /* not executeable */
        return -RT_ERROR;
    }

    off = eheader.e_phoff;
    for (i = 0; i < eheader.e_phnum; i++, off += sizeof pheader) {
        check_off(off, len);
        fseek(fp, off, SEEK_SET);
        read_len = load_fread(&pheader, 1, sizeof pheader, fp);
        check_read(read_len, sizeof pheader);

        if (pheader.p_type == PT_LOAD) {
            if (pheader.p_filesz > pheader.p_memsz)
            {
                return -RT_ERROR;
            }
            if (load_addr)
                lwp->text_entry = load_addr;
            else
            {
#ifdef RT_USING_CACHE
                lwp->text_entry = (rt_uint8_t *)rt_malloc_align(pheader.p_memsz, RT_CPU_CACHE_LINE_SZ);
#else
                lwp->text_entry = (rt_uint8_t *)rt_malloc(pheader.p_memsz);
#endif
                if (lwp->text_entry == RT_NULL)
                {
                    dbg_log(DBG_ERROR, "alloc text memory faild!\n");
                    result = -RT_ENOMEM;
                    goto _exit;
                }
                else
                {
                    dbg_log(DBG_LOG, "lwp text malloc : %p, size: %d!\n", lwp->text_entry, lwp->text_size);
                }
                check_off(pheader.p_offset, len);
                fseek(fp, pheader.p_offset, SEEK_SET);
                read_len = load_fread(lwp->text_entry, 1, pheader.p_filesz, fp);
                check_read(read_len, pheader.p_filesz);
            }
            if (pheader.p_filesz < pheader.p_memsz)
            {
                memset(lwp->text_entry + pheader.p_filesz, 0, pheader.p_memsz - pheader.p_filesz);
            }
            break;
        }
    }

    /* section info */
    off = eheader.e_shoff;
    /* find section string tabel */
    check_off(off, len);
    fseek(fp, off + (sizeof sheader) * eheader.e_shstrndx, SEEK_SET);
    read_len = load_fread(&sheader, 1, sizeof sheader, fp);
    check_read(read_len, sizeof sheader);

    p_section_str = (char*)rt_malloc(sheader.sh_size);
    if (!p_section_str) {
        rt_kprintf("ERROR: Malloc error!\n");
        result = -ENOMEM;
        goto _exit;
    }
    check_off(sheader.sh_offset, len);
    fseek(fp, sheader.sh_offset, SEEK_SET);
    read_len = load_fread(p_section_str, 1, sheader.sh_size, fp);
    check_read(read_len, sheader.sh_size);

    text_vaddr = 0;
    check_off(off, len);
    fseek(fp, off, SEEK_SET);
    for (i = 0; i < eheader.e_shnum; i++, off += sizeof sheader) {
        read_len = load_fread(&sheader, 1, sizeof sheader, fp);
        check_read(read_len, sizeof sheader);

        if (strcmp(p_section_str + sheader.sh_name, "text") == 0)
        {
            text_vaddr = sheader.sh_addr;
            lwp->text_size = sheader.sh_size;
        }
        else if (strcmp(p_section_str + sheader.sh_name, "data") == 0)
        {
            lwp->data = lwp->text_entry + sheader.sh_addr;
            lwp->data_size = pheader.p_memsz - sheader.sh_addr;
        }
    }
    lwp->data -= text_vaddr;
    dprintf("lwp->text_entry = 0x%p\n", lwp->text_entry);
    dprintf("lwp->text_size = 0x%p\n", lwp->text_size);
    dprintf("lwp->data = 0x%p\n", lwp->data);
    dprintf("lwp->data_size = 0x%p\n", lwp->data_size);
#ifdef RT_USING_CACHE
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, lwp->text_entry, lwp->text_size);
    rt_hw_cpu_icache_ops(RT_HW_CACHE_INVALIDATE, lwp->text_entry, lwp->text_size);
#endif

_exit:
    if (p_section_str)
    {
        rt_free(p_section_str);
    }
    if (result != RT_EOK)
    {
        if (lwp->lwp_type == LWP_TYPE_DYN_ADDR)
        {
            dbg_log(DBG_ERROR, "lwp dynamic load faild, %d\n", result);
            if (lwp->text_entry)
            {
                dbg_log(DBG_LOG, "lwp text free: %p\n", lwp->text_entry);
#ifdef RT_USING_CACHE
                rt_free_align(lwp->text_entry);
#else
                rt_free(lwp->text_entry);
#endif
            }
        }
    }
    return result;
}


static int lwp_load(const char *filename, struct rt_lwp *lwp, uint8_t *load_addr, size_t addr_size)
{
    uint8_t *ptr;
    int ret = -1;
    int len;
    FILE *fp = 0;

    /* check file name */
    RT_ASSERT(filename != RT_NULL);
    /* check lwp control block */
    RT_ASSERT(lwp != RT_NULL);

    if (load_addr != RT_NULL)
    {
        lwp->lwp_type = LWP_TYPE_FIX_ADDR;
        ptr = load_addr;
    }
    else
    {
        lwp->lwp_type = LWP_TYPE_DYN_ADDR;
        ptr = RT_NULL;
    }

    fp = fopen(filename, "rb");
    if (!fp) {
        rt_kprintf("ERROR: Can't open elf file %s!\n", filename);
        goto out;
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    if (!len) {
        rt_kprintf("ERROR: File %s size error!\n", filename);
        goto out;
    }

    fseek(fp, 0, SEEK_SET);

    ret = load_elf(fp, len, lwp, ptr);
    if (ret != RT_EOK)
    {
        rt_kprintf("lwp load ret = %d\n", ret);
    }

out:
    if (fp) {
        fclose(fp);
    }
    return ret;
}

static void lwp_cleanup(struct rt_thread *tid)
{
    struct rt_lwp *lwp;

    dbg_log(DBG_INFO, "thread: %s, stack_addr: %08X\n", tid->name, tid->stack_addr);

    lwp = (struct rt_lwp *)tid->lwp;

    if (lwp->lwp_type == LWP_TYPE_DYN_ADDR)
    {
        dbg_log(DBG_INFO, "dynamic lwp\n");
        if (lwp->text_entry)
        {
            dbg_log(DBG_LOG, "lwp text free: %p\n", lwp->text_entry);
#ifdef RT_USING_CACHE
            rt_free_align(lwp->text_entry);
#else
            rt_free(lwp->text_entry);
#endif
        }
        /*
        if (lwp->data)
        {
            dbg_log(DBG_LOG, "lwp data free: %p\n", lwp->data);
            rt_free(lwp->data);
        }
        */
    }

    dbg_log(DBG_LOG, "lwp free memory pages\n");
    rt_lwp_mem_deinit(lwp);

    /* cleanup fd table */
    rt_free(lwp->fdt.fds);
    rt_free(lwp->args);

    dbg_log(DBG_LOG, "lwp free: %p\n", lwp);
    rt_free(lwp);

    /* TODO: cleanup fd table */
}

static void lwp_thread(void *parameter)
{
    rt_thread_t tid;
    struct rt_lwp *lwp;

    lwp = (struct rt_lwp *)parameter;
    rt_lwp_mem_init(lwp);
    tid = rt_thread_self();
    tid->lwp = lwp;
    tid->cleanup = lwp_cleanup;

    lwp_user_entry(lwp->args, lwp->text_entry, lwp->data);
}

struct rt_lwp *rt_lwp_self(void)
{
    return (struct rt_lwp *)rt_thread_self()->lwp;
}

int exec(char *filename, int argc, char **argv)
{
    struct rt_lwp *lwp;
    int result;

    if (filename == RT_NULL)
        return -RT_ERROR;

    lwp = (struct rt_lwp *)rt_malloc(sizeof(struct rt_lwp));
    if (lwp == RT_NULL)
    {
        dbg_log(DBG_ERROR, "lwp struct out of memory!\n");
        return -RT_ENOMEM;
    }
    dbg_log(DBG_INFO, "lwp malloc : %p, size: %d!\n", lwp, sizeof(struct rt_lwp));

    rt_memset(lwp, 0, sizeof(*lwp));
    if (lwp_argscopy(lwp, argc, argv) != 0)
    {
        rt_free(lwp);
        return -ENOMEM;
    }

    result = lwp_load(filename, lwp, RT_NULL, 0);
    if (result == RT_EOK)
    {
        rt_thread_t tid;

        tid = rt_thread_create("user", lwp_thread, (void *)lwp,
                               1024 * 4, 2, 200);
        if (tid != RT_NULL)
        {
            dbg_log(DBG_LOG, "lwp kernel => (0x%08x, 0x%08x)\n", (rt_uint32_t)tid->stack_addr, (rt_uint32_t)tid->stack_addr + tid->stack_size);
            rt_thread_startup(tid);
            return RT_EOK;
        }
        else
        {
#ifdef RT_USING_CACHE
            rt_free_align(lwp->text_entry);
#else
            rt_free(lwp->text_entry);
#endif
            /*
            rt_free(lwp->data);
            */
        }
    }

    rt_free(lwp->args);
    rt_free(lwp);

    return -RT_ERROR;
}
