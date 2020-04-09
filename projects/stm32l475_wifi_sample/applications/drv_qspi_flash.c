/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-27     zylx         first version
 */
 
#include <board.h>
#include <drv_qspi.h>
#include <rtdevice.h>
#include <rthw.h>
#include <finsh.h>

#ifdef BSP_USING_QSPI_FLASH

#include "spi_flash.h"
#include "spi_flash_sfud.h"

//#define DRV_DEBUG
#define LOG_TAG             "drv.qspi"
#include <drv_log.h>

char w25qxx_read_status_register2(struct rt_qspi_device *device)
{
    /* 0x35 read status register2 */
    char instruction = 0x35, status;

    rt_qspi_send_then_recv(device, &instruction, 1, &status, 1);

    return status;
}

void w25qxx_write_enable(struct rt_qspi_device *device)
{
    /* 0x06 write enable */
    char instruction = 0x06;

    rt_qspi_send(device, &instruction, 1);
}

void w25qxx_enter_qspi_mode(struct rt_qspi_device *device)
{
    char status = 0;
    /* 0x38 enter qspi mode */
    char instruction = 0x38;
    char write_status2_buf[2] = {0};

    /* 0x31 write status register2 */
    write_status2_buf[0] = 0x31;

    status = w25qxx_read_status_register2(device);
    if (!(status & 0x02))
    {
        status |= 1 << 1;
        w25qxx_write_enable(device);
        write_status2_buf[1] = status;
        rt_qspi_send(device, &write_status2_buf, 2);
        rt_qspi_send(device, &instruction, 1);
        rt_kprintf("flash already enter qspi mode\n");
        rt_thread_mdelay(10);
    }
}

static int rt_hw_qspi_flash_with_sfud_init(void)
{
    stm32_qspi_bus_attach_device("qspi1", "qspi10", RT_NULL, 4, w25qxx_enter_qspi_mode, RT_NULL);
    
    /* init w25q128 */
    if (RT_NULL == rt_sfud_flash_probe("W25Q128", "qspi10"))
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_qspi_flash_with_sfud_init);

#if defined(RT_USING_DFS_ELMFAT) && !defined(BSP_USING_SDCARD)
#include <dfs_fs.h>
#include <fal.h>

#define FS_PARTITION_NAME  "filesystem"

int mnt_init(void)
{
    rt_thread_delay(RT_TICK_PER_SECOND);
    rt_uint32_t fal_flag = 0;

    #if defined(PKG_USING_FAL)
        fal_flag = 1;
        #define BLK_DEV_NAME  "filesystem"
    #else
        #define BLK_DEV_NAME  "W25Q128"

    #endif


    if(fal_flag)
    {
        struct fal_blk_device *blk_dev;
        fal_init();

        /* create block device */
        blk_dev = (struct fal_blk_device *)fal_blk_device_create(FS_PARTITION_NAME);
        if(blk_dev == RT_NULL)
            LOG_E("Create a block device on '%s' partition failed!", FS_PARTITION_NAME);
        else
            LOG_I("Create a block device on the '%s' partition successful!", FS_PARTITION_NAME);
    }

    if (dfs_mount(BLK_DEV_NAME, "/", "elm", 0, 0) == 0)
    {
        LOG_I("elmfat file system initialization done!");
    }
    else
    {
        if(dfs_mkfs("elm", BLK_DEV_NAME) == 0)
        {
            if (dfs_mount(BLK_DEV_NAME, "/", "elm", 0, 0) == 0)
            {
                LOG_I("elmfat file system initialization done!");
            }
            else
            {
                LOG_E("elmfat file system initialization failed!");
            }
        }
    }

    return 0;
}
INIT_ENV_EXPORT(mnt_init);

#endif /* defined(RT_USING_DFS_ELMFAT) && !defined(BSP_USING_SDCARD) */
#endif /* BSP_USING_QSPI_FLASH */
