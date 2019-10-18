#include "rtdevice.h"
#include "rtthread.h"
#include "board.h"
#include "drv_qspi.h"
#include "spi_flash_sfud.h"
#include "sfud.h"

#define QSPI_BUD_NAME       "qspi1"
#define QSPI_DEVICE_NAME    "qspi10"
#define W25Q_FLASH_NAME     "W25Q128"

#define QSPI_CS_PIN         GET_PIN(E, 11)

rt_uint8_t wData[4096] = {"QSPI bus write data to W25Q flash."};
rt_uint8_t rData[4096];

static int rt_hw_spi_flash_init(void)
{
//    if(rt_hw_spi_device_attach(QSPI_BUD_NAME, QSPI_DEVICE_NAME, GPIOE, GPIO_PIN_11) != RT_EOK)
//        return -RT_ERROR;

    if(stm32_qspi_bus_attach_device(QSPI_BUD_NAME, QSPI_DEVICE_NAME, (rt_uint32_t)QSPI_CS_PIN, 1, RT_NULL, RT_NULL) != RT_EOK)
        return -RT_ERROR;

#ifdef RT_USING_SFUD
    if(rt_sfud_flash_probe(W25Q_FLASH_NAME, QSPI_DEVICE_NAME) == RT_NULL)
        return -RT_ERROR;
#endif

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_spi_flash_init);


static void sfud_w25q_sample(void)
{
    rt_spi_flash_device_t flash_dev;
    sfud_flash_t sfud_dev;
    struct rt_device_blk_geometry geometry;

    // 1- use sfud api
    rt_kprintf("\n 1 - Use SFUD API \n");

    sfud_dev = rt_sfud_flash_find_by_dev_name(W25Q_FLASH_NAME);
    if(sfud_dev == RT_NULL){
        rt_kprintf("sfud can't find %s device.\n", W25Q_FLASH_NAME);
    }else{
        rt_kprintf("sfud device name: %s, sector_count: %d, bytes_per_sector: %d, block_size: %d.\n", 
                    sfud_dev->name, sfud_dev->chip.capacity / sfud_dev->chip.erase_gran, 
                    sfud_dev->chip.erase_gran, sfud_dev->chip.erase_gran);

        if(sfud_erase_write(sfud_dev, 0x002000, sizeof(wData), wData) == SFUD_SUCCESS)
            rt_kprintf("sfud api write data to w25q128(address:0x2000) success.\n");

        if(sfud_read(sfud_dev, 0x002000, sizeof(rData), rData) == SFUD_SUCCESS)
            rt_kprintf("sfud api read data from w25q128(address:0x2000) is:%s\n", rData);
    }

    // 2- use rt_device api
    rt_kprintf("\n 2 - Use rt_device API \n");

    flash_dev = (rt_spi_flash_device_t)rt_device_find(W25Q_FLASH_NAME);
    if(flash_dev == RT_NULL){
        rt_kprintf("rt_device api can't find %s device.\n", W25Q_FLASH_NAME);
    }else{
        rt_device_open(&flash_dev->flash_device, RT_DEVICE_OFLAG_OPEN);

        if(rt_device_control(&flash_dev->flash_device, RT_DEVICE_CTRL_BLK_GETGEOME, &geometry) == RT_EOK)
            rt_kprintf("spi flash device name: %s, sector_count: %d, bytes_per_sector: %d, block_size: %d.\n", 
                    flash_dev->flash_device.parent.name, geometry.sector_count, geometry.bytes_per_sector, geometry.block_size);

        if(rt_device_write(&flash_dev->flash_device, 0x03, wData, 1) > 0)
            rt_kprintf("rt_device api write data to w25q128(address:0x3000) success.\n");

        if(rt_device_read(&flash_dev->flash_device, 0x03, rData, 1) > 0)
            rt_kprintf("rt_device api read data from w25q128(address:0x3000) is:%s\n", rData);

        rt_device_close(&flash_dev->flash_device);
    }
}
MSH_CMD_EXPORT(sfud_w25q_sample, sfud w25q128 sample);


static void qspi_w25q_sample(void)
{
    struct rt_qspi_device *qspi_dev_w25q = RT_NULL;
    struct rt_qspi_message message;
    struct rt_qspi_configuration config;

    rt_uint8_t w25q_read_id[4] = {0x90, 0x00, 0x00, 0x00};
    rt_uint8_t w25q_read_data[4] = {0x03, 0x00, 0x10, 0x00};
    rt_uint8_t w25q_erase_sector[4] = {0x20, 0x00, 0x10, 0x00};
    rt_uint8_t w25q_write_enable = 0x06;
    rt_uint8_t W25X_ReadStatusReg1 = 0x05;
    rt_uint8_t id[2] = {0};
    rt_uint8_t status = 1;

    qspi_dev_w25q = (struct rt_qspi_device *)rt_device_find(QSPI_DEVICE_NAME);
    if(qspi_dev_w25q == RT_NULL){
        rt_kprintf("qspi sample run failed! can't find %s device!\n", QSPI_DEVICE_NAME);
    }else{
        // config w25q qspi
        config.parent.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
        config.parent.data_width = 8;
        config.parent.max_hz = 50 * 1000 * 1000;
        config.medium_size = 16 * 1024 * 1024;
        config.ddr_mode = 0;
        config.qspi_dl_width = 4;
        rt_qspi_configure(qspi_dev_w25q, &config);
        rt_kprintf("qspi10 config finish.\n");

        // read w25q id
        rt_qspi_send_then_recv(qspi_dev_w25q, w25q_read_id, 4, id, 2);
        rt_kprintf("qspi10 read w25q ID is:%2x%2x\n", id[0], id[1]);

        // erase sector address 4096
        rt_qspi_send(qspi_dev_w25q, &w25q_write_enable, 1);
        rt_qspi_send(qspi_dev_w25q, w25q_erase_sector, 4);
        // wait transfer finish
        while((status & 0x01) == 0x01)
            rt_qspi_send_then_recv(qspi_dev_w25q, &W25X_ReadStatusReg1, 1, &status, 1);
        rt_kprintf("qspi10 erase w25q sector(address:0x1000) success.\n");

        // write data to w25q address 4096
        rt_qspi_send(qspi_dev_w25q, &w25q_write_enable, 1);
        message.parent.send_buf = wData;
        message.parent.recv_buf = RT_NULL;
        message.parent.length = 64;
        message.parent.next = RT_NULL;
        message.parent.cs_take = 1;
        message.parent.cs_release = 1;
        message.instruction.content = 0X32;
        message.instruction.qspi_lines = 1;
        message.address.content = 0X00001000;
        message.address.size = 24;
        message.address.qspi_lines = 1;
        message.dummy_cycles = 0;
        message.qspi_data_lines = 4;
        rt_qspi_transfer_message(qspi_dev_w25q, &message);
        // wait transfer finish
        status = 1;
        while((status & 0x01) == 0x01)
            rt_qspi_send_then_recv(qspi_dev_w25q, &W25X_ReadStatusReg1, 1, &status, 1);
        rt_kprintf("qspi10 write data to w25q(address:0x1000) success.\n");

        // read data from w25q address 4096
        rt_qspi_send_then_recv(qspi_dev_w25q, w25q_read_data, 4, rData, 64);
        rt_kprintf("qspi10 read data from w25q(address:0x1000) is:%s\n", rData);
    }
}
MSH_CMD_EXPORT(qspi_w25q_sample, qspi w25q128 sample);

