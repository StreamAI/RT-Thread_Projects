#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <msh.h>
#include <wlan_mgnt.h>
#include <wifi_config.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define APP_VERSION  "2.0.0"

/* 将中断向量表起始地址重新设置为 app 分区的起始地址 */
static int ota_app_vtor_reconfig(void)
{
    #define NVIC_VTOR_MASK   0xFFFFFF80
    #define RT_APP_PART_ADDR 0x08010000
    /* 根据应用设置向量表 */
    SCB->VTOR = RT_APP_PART_ADDR & NVIC_VTOR_MASK;

    return 0;
}
INIT_BOARD_EXPORT(ota_app_vtor_reconfig);

int main(void)
{
    /* 初始化 wlan 自动连接配置 */
    wlan_autoconnect_init();
    /* 使能 wlan 自动连接功能 */
    rt_wlan_config_autoreconnect(RT_TRUE);

    /* 打印当前软件版本信息 */
    LOG_I("The current version of APP firmware is %s\n", APP_VERSION);

    return 0;
}
