#include "rtdevice.h"
#include "rtthread.h"
#include "board.h"
#include "dfs_posix.h"


#define BEEP_PIN    GET_PIN(B, 2)

static void devfs_sample(void)
{
    int fd;
    DIR *dirp;
    struct dirent *d;
    struct rt_device_pin_mode beepmode;
    struct rt_device_pin_status beepstatus, tempstatus;

    dirp = opendir("/dev");
    if(dirp == RT_NULL){
        rt_kprintf("open directory error!\n");
    }else{
        while ((d = readdir(dirp)) != RT_NULL){
            rt_kprintf("found device: %s\n", d->d_name);
        }
        closedir(dirp);
    }

    beepmode.pin = beepstatus.pin = tempstatus.pin = BEEP_PIN;
    beepmode.mode = PIN_MODE_OUTPUT;
    beepstatus.status = PIN_HIGH;

    rt_kprintf("Control beep via devfs.\n");

    fd = open("/dev/pin", RT_DEVICE_OFLAG_OPEN);
    if(fd >= 0){
        if(ioctl(fd, 0, &beepmode) == 0)
            rt_kprintf("Beep pin mode config finish.\n");

        if(write(fd, &beepstatus, sizeof(beepstatus)) == sizeof(beepstatus))
            rt_kprintf("Beep turn on.\n");

        if(read(fd, &tempstatus, sizeof(tempstatus)) == sizeof(tempstatus))
            rt_kprintf("Beep pin status: %d.\n", tempstatus.status);

        close(fd);
    }

    rt_thread_mdelay(3000);

    beepstatus.status = PIN_LOW;

    /* 打开设备文件的标识RT_DEVICE_OFLAG_OPEN */
    fd = open("/dev/pin", RT_DEVICE_OFLAG_OPEN);
    if(fd >= 0){
        if(write(fd, &beepstatus, sizeof(beepstatus)) == sizeof(beepstatus))
            rt_kprintf("Beep turn off.\n");

        if(read(fd, &tempstatus, sizeof(tempstatus)) == sizeof(tempstatus))
            rt_kprintf("Beep pin status: %d.\n", tempstatus.status);

        close(fd);
    }
}
MSH_CMD_EXPORT(devfs_sample, devfs sample);


#include "drv_qspi.h"
#include "spi_flash_sfud.h"

#define QSPI_BUD_NAME       "qspi1"
#define QSPI_DEVICE_NAME    "qspi10"
#define W25Q_FLASH_NAME     "W25Q128"

#define QSPI_CS_PIN         GET_PIN(E, 11)

static int rt_hw_spi_flash_init(void)
{
    if(stm32_qspi_bus_attach_device(QSPI_BUD_NAME, QSPI_DEVICE_NAME, (rt_uint32_t)QSPI_CS_PIN, 1, RT_NULL, RT_NULL) != RT_EOK)
        return -RT_ERROR;

    if(rt_sfud_flash_probe(W25Q_FLASH_NAME, QSPI_DEVICE_NAME) == RT_NULL)
        return -RT_ERROR;

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_spi_flash_init);

static void elmfat_sample(void)
{
    int fd, size;
    struct statfs elm_stat;
    char str[] = "elmfat mount to W25Q flash.", buf[80];

    if(dfs_mkfs("elm", W25Q_FLASH_NAME) == 0)
        rt_kprintf("make elmfat filesystem success.\n");

    if(dfs_mount(W25Q_FLASH_NAME, "/", "elm", 0, 0) == 0)
        rt_kprintf("elmfat filesystem mount success.\n");

    if(statfs("/", &elm_stat) == 0)
        rt_kprintf("elmfat filesystem block size: %d, total blocks: %d, free blocks: %d.\n", 
                    elm_stat.f_bsize, elm_stat.f_blocks, elm_stat.f_bfree);

    if(mkdir("/user", 0x777) == 0)
        rt_kprintf("make a directory: '/user'.\n");

    rt_kprintf("Write string '%s' to /user/test.txt.\n", str);

    /* Open the file in create and read-write mode, create the file if it does not exist*/
    fd = open("/user/test.txt", O_WRONLY | O_CREAT);
    if (fd >= 0)
    {
        if(write(fd, str, sizeof(str)) == sizeof(str))
            rt_kprintf("Write data done.\n");

        close(fd);   
    }

    /* Open file in read-only mode */
    fd = open("/user/test.txt", O_RDONLY);
    if (fd >= 0)
    {
        size = read(fd, buf, sizeof(buf));

        close(fd);

        if(size == sizeof(str))
            rt_kprintf("Read data from file test.txt(size: %d): %s \n", size, buf);
    }
}
MSH_CMD_EXPORT(elmfat_sample, elmfat sample);
