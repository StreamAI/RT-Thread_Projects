#include "rtdevice.h"
#include "rtthread.h"
#include "board.h"
#include "drv_qspi.h"

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

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_spi_flash_init);

static void qspi_dev_sample(void)
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
        rt_kprintf("qspi10 erase w25q data success.\n");

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
        rt_kprintf("qspi10 write data to w25q success.\n");

        // read data from w25q address 4096
        rt_qspi_send_then_recv(qspi_dev_w25q, w25q_read_data, 4, rData, 64);
        rt_kprintf("qspi10 read w25q data is:%s\n", rData);
    }
}
MSH_CMD_EXPORT(qspi_dev_sample, qspi device sample);

