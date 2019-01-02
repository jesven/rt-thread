#include <rtthread.h>
#include <finsh.h>
#include "dfs_file.h"

#include "utest.h"
#include "utest_assert.h"

static struct dfs_fd fd;
static const char write_buf[] = "Hello RT-Thread!";

#define TEST_MNT_PATH  TC_FS_DFS_MOUNT_PATH_
#define TEST_BLOCK_DEV TC_FS_DFS_BLOCK_DEV_NAME
#define TEST_FS        TC_FS_DFS_NAME

#define TEST_FILE      TEST_MNT_PATH"/RT-Thread.txt"


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

static void test_dfs_mount(void)
{
    rt_err_t rst = RT_EOK;
    dfs_unmount(TEST_MNT_PATH);
    rst = dfs_mount(TEST_BLOCK_DEV, TEST_MNT_PATH, TEST_FS, 0, 0);
    LOG_D("dfs mount rst: %d", rst);
	uassert_true(rst == 0);
}

static void test_dfs_open(void)
{
    uassert_true(dfs_file_open(&fd, TEST_FILE, O_CREAT | O_RDWR) == 0);
}

static rt_err_t test_write(void)
{
    rt_err_t rst = RT_EOK;

    if (dfs_file_lseek(&fd, 0) != 0)
    {
        rst = -RT_ERROR;
        LOG_D("lseek failed. %d", dfs_file_lseek(&fd, 0));
        return rst;
    }

    rst = dfs_file_write(&fd, write_buf, rt_strlen(write_buf));
    if (rst < 0)
    {
        LOG_D("dfs write failed! rst = %d", rst);
        rst = -RT_ERROR;
    }
    else if (rst < rt_strlen(write_buf))
    {
        rst = -RT_ERROR;
        LOG_D("Did not write all data! rst = %d", rst);
    }
    else
    {
        rst = RT_EOK;
    }
    return rst;
}

static void test_dfs_write(void)
{
    uassert_true(test_write() == RT_EOK);
}

static rt_err_t test_read(void)
{
    rt_err_t rst = RT_EOK;

    char read_buf[24];

    if (dfs_file_lseek(&fd, 0) != 0)
    {
        rst = -RT_ERROR;
        LOG_D("lseek failed. %d", dfs_file_lseek(&fd, 0));
        return rst;
    }

    rst = dfs_file_read(&fd, read_buf, rt_strlen(write_buf));
    if (rst < 0)
    {
        LOG_D("dfs read failed");
        rst = -RT_ERROR;
        return rst;
    }

    read_buf[rst] = '\0';

    if (rt_strcmp(write_buf, read_buf))
    {
        LOG_D("dfs read data err, readbuf:%s; rst:%d", read_buf, rst);
        rst = -RT_ERROR;
        return rst;
    }
    rst = RT_EOK;

    return rst;
}

static void test_dfs_read(void)
{
    uassert_true(test_read() == RT_EOK);
}

static void test_dfs_close(void)
{
    uassert_true(dfs_file_close(&fd) == 0);
}

static void test_dfs_mkdir(void)
{

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
    UTEST_UNIT_RUN(test_dfs_mount);
    UTEST_UNIT_RUN(test_dfs_open);
    UTEST_UNIT_RUN(test_dfs_write);
    UTEST_UNIT_RUN(test_dfs_read);
    UTEST_UNIT_RUN(test_dfs_close);
    UTEST_UNIT_RUN(test_dfs_mkdir);
}
UTEST_TC_EXPORT(testcase, "components.filesystem.dfs.dfs_api_tc", utest_tc_init, utest_tc_cleanup, 30);
