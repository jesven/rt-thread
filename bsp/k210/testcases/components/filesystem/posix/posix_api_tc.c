#include <rtthread.h>
#include <finsh.h>
#include <dfs_posix.h>

#include "utest.h"
#include "utest_assert.h"

int fd = 0;
static const char write_buf[] = "Hello RT-Thread!";

#define TEST_MNT_PATH TC_FS_POSIX_MOUNT_PATH_
#define TEST_BLOCK_DEV TC_FS_POSIX_BLOCK_DEV_NAME
#define TEST_FS        TC_FS_POSIX_NAME

#define TEST_DIR       TEST_MNT_PATH"/test"
#define TEST_FILE      TEST_MNT_PATH"/RT-Thread.txt"
#define TEST_DIR_FILE  TEST_DIR"/RT-Thread.txt"

/**
 * mkfs --> mount --> open --> write --> open --> close --> unlink --> 
 * mkdir --> opendir --> readdir --> telldir --> 
*/

static void test_mkfs(void)
{
    rt_err_t rst = RT_EOK;
    rst = dfs_mkfs(TEST_FS, TEST_BLOCK_DEV);
    if (rst != 0)
    {
        LOG_E("mkfs failed!");
    }
	uassert_true(rst == 0);
}

static void test_mount(void)
{
    rt_err_t rst = RT_EOK;
    dfs_unmount(TEST_MNT_PATH);
    rst = dfs_mount(TEST_BLOCK_DEV, TEST_MNT_PATH, TEST_FS, 0, 0);
    LOG_D("dfs mount rst: %d", rst);
	uassert_true(rst == 0);
}

static void test_posix_open(void)
{
    fd = open(TEST_FILE, O_CREAT | O_RDWR);
    uassert_false(fd == 0);
}

static rt_err_t test_write(void)
{
    rt_err_t rst = RT_EOK;

    if (lseek(fd, 0, 0) != 0)
    {
        rst = -RT_ERROR;
        LOG_D("lseek failed.");
        close(fd);
        return rst;
    }

    rst = write(fd, write_buf, rt_strlen(write_buf));
    if (rst < 0)
    {
        LOG_D("dfs write failed! rst = %d", rst);
        rst = -RT_ERROR;
        close(fd);
    }
    else if (rst != rt_strlen(write_buf))
    {
        rst = -RT_ERROR;
        close(fd);
        LOG_D("Did not write all data! rst = %d", rst);
    }
    else
    {
        rst = RT_EOK;
    }
    return rst;
}

static void test_posix_write(void)
{
    uassert_true(test_write() == RT_EOK);
}

static rt_err_t test_read(void)
{
    rt_err_t rst = RT_EOK;

    char read_buf[24];

    if (lseek(fd, 0, 0) != 0)
    {
        rst = -RT_ERROR;
        close(fd);
        LOG_D("lseek failed.");
        return rst;
    }

    rst = read(fd, read_buf, rt_strlen(write_buf));
    if (rst < 0)
    {
        LOG_D("dfs read failed");
        rst = -RT_ERROR;
        close(fd);
        return rst;
    }

    read_buf[rst] = '\0';

    if (rt_strcmp(write_buf, read_buf))
    {
        LOG_D("dfs read data err, readbuf:%s; rst:%d", read_buf, rst);
        rst = -RT_ERROR;
        close(fd);
        return rst;
    }
    rst = RT_EOK;

    return rst;
}

static void test_posix_read(void)
{
    uassert_true(test_read() == RT_EOK);
}

static void test_posix_close(void)
{
    uassert_true(close(fd) == 0);
}

static rt_err_t test_mkdir(void)
{
    rt_err_t rst = RT_EOK;
    rst = mkdir(TEST_DIR, S_IRWXG);
    if (rst != 0)
    {
        LOG_D("mkdir failed!");
        rst = -RT_ERROR;
        return rst;
    }

    fd = open(TEST_DIR_FILE, O_CREAT | O_RDWR);
    if (fd < 0)
    {
        rst = -RT_ERROR;
        return rst;
    }

    if (test_write() != RT_EOK)
    {
        rst = -RT_ERROR;
        return rst;
    }

    if (test_read() != RT_EOK)
    {
        rst = -RT_ERROR;
        return rst;
    }

    rst = RT_EOK;
    return rst;
}

static void test_posix_mkdir(void)
{
    uassert_true(test_mkdir() == RT_EOK);
}

static rt_err_t test_readdir(void)
{
    rt_err_t rst = RT_EOK;
    DIR *dir;
    struct dirent *entry;
    dir = opendir(TEST_DIR);
    if (dir == NULL)
    {
        rst = -RT_ERROR;
        return rst;
    }
    entry = readdir(dir);
    if (entry == NULL)
    {
        rst = -RT_ERROR;
        return rst;
    }
    closedir(dir);

    rst = RT_EOK;
    return rst;
}

static void test_posix_readdir(void)
{
    uassert_true(test_readdir() == RT_EOK);
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
    LOG_I("in testcase func...");

    UTEST_UNIT_RUN(test_mkfs);
    UTEST_UNIT_RUN(test_mount);
    UTEST_UNIT_RUN(test_posix_open);
    UTEST_UNIT_RUN(test_posix_write);
    UTEST_UNIT_RUN(test_posix_read);
    UTEST_UNIT_RUN(test_posix_close);
    UTEST_UNIT_RUN(test_posix_mkdir);
    UTEST_UNIT_RUN(test_posix_readdir);
}
UTEST_TC_EXPORT(testcase, "components.filesystem.posix.posix_api_tc", utest_tc_init, utest_tc_cleanup, 30);
