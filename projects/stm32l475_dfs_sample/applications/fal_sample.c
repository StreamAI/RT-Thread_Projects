#include "rtthread.h"
#include "rtdevice.h"
#include "board.h"
#include "fal.h"


#define BUF_SIZE 1024

static int fal_test(const char *partiton_name)
{
    int ret;
    int i, j, len;
    uint8_t buf[BUF_SIZE];
    const struct fal_flash_dev *flash_dev = RT_NULL;
    const struct fal_partition *partition = RT_NULL;

    if (!partiton_name)
    {
        rt_kprintf("Input param partition name is null!\n");
        return -1;
    }

    partition = fal_partition_find(partiton_name);
    if (partition == RT_NULL)
    {
        rt_kprintf("Find partition (%s) failed!\n", partiton_name);
        ret = -1;
        return ret;
    }

    flash_dev = fal_flash_device_find(partition->flash_name);
    if (flash_dev == RT_NULL)
    {
        rt_kprintf("Find flash device (%s) failed!\n", partition->flash_name);
        ret = -1;
        return ret;
    }

    rt_kprintf("Flash device : %s   "
               "Flash size : %dK   \n"
               "Partition : %s   "
               "Partition size: %dK\n", 
                partition->flash_name, 
                flash_dev->len/1024,
                partition->name,
                partition->len/1024);

    /* erase all partition */
    ret = fal_partition_erase_all(partition);
    if (ret < 0)
    {
        rt_kprintf("Partition (%s) erase failed!\n", partition->name);
        ret = -1;
        return ret;
    }
    rt_kprintf("Erase (%s) partition finish!\n", partiton_name);

    /* read the specified partition and check data */
    for (i = 0; i < partition->len;)
    {
        rt_memset(buf, 0x00, BUF_SIZE);

        len = (partition->len - i) > BUF_SIZE ? BUF_SIZE : (partition->len - i);

        ret = fal_partition_read(partition, i, buf, len);
        if (ret < 0)
        {
            rt_kprintf("Partition (%s) read failed!\n", partition->name);
            ret = -1;
            return ret;
        }

        for(j = 0; j < len; j++)
        {
            if (buf[j] != 0xFF)
            {
                rt_kprintf("The erase operation did not really succeed!\n");
                ret = -1;
                return ret;
            }
        }
        i += len;
    }

    /* write 0x00 to the specified partition */
    for (i = 0; i < partition->len;)
    {
        rt_memset(buf, 0x00, BUF_SIZE);

        len = (partition->len - i) > BUF_SIZE ? BUF_SIZE : (partition->len - i);

        ret = fal_partition_write(partition, i, buf, len);
        if (ret < 0)
        {
            rt_kprintf("Partition (%s) write failed!\n", partition->name);
            ret = -1;
            return ret;
        }

        i += len;
    }
    rt_kprintf("Write (%s) partition finish! Write size %d(%dK).\n", partiton_name, i, i/1024);

    /* read the specified partition and check data */
    for (i = 0; i < partition->len;)
    {
        rt_memset(buf, 0xFF, BUF_SIZE);

        len = (partition->len - i) > BUF_SIZE ? BUF_SIZE : (partition->len - i);

        ret = fal_partition_read(partition, i, buf, len);
        if (ret < 0)
        {
            rt_kprintf("Partition (%s) read failed!\n", partition->name);
            ret = -1;
            return ret;
        }

        for(j = 0; j < len; j++)
        {
            if (buf[j] != 0x00)
            {
                rt_kprintf("The write operation did not really succeed!\n");
                ret = -1;
                return ret;
            }
        }

        i += len;
    }

    ret = 0;
    return ret;
}

static void fal_sample(void)
{
    /* 1- init */
    fal_init();

    if (fal_test("param") == 0)
    {
        rt_kprintf("Fal partition (%s) test success!\n", "param");
    }
    else
    {
        rt_kprintf("Fal partition (%s) test failed!\n", "param");
    }

    if (fal_test("download") == 0)
    {
        rt_kprintf("Fal partition (%s) test success!\n", "download");
    }
    else
    {
        rt_kprintf("Fal partition (%s) test failed!\n", "download");
    }
}

MSH_CMD_EXPORT(fal_sample, fal sample);
