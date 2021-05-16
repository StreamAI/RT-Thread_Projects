#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <msh.h>
#include <wlan_mgnt.h>
#include <wifi_config.h>
#include <onenet.h>
#include <sensor.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define APP_VERSION  "3.0.0"

/* defined the LED_R pin: PE7 */
#define LED_R    GET_PIN(E, 7)

/* defined aht10 sensor name */
#define SENSOR_TEMP_NAME    "temp_aht10"
#define SENSOR_HUMI_NAME    "humi_aht10"


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

/* upload temperature and humidity datapoint */
static void onenet_upload_datapoint_thread(void *parameter)
{
    double temp_value, humi_value;

    /* sensor设备对象与sensor数据类型 */
    rt_device_t sensor_temp, sensor_humi;
    struct rt_sensor_data temp_data, humi_data;

    /* 发现并打开温湿度传感器设备 */
    sensor_temp = rt_device_find(SENSOR_TEMP_NAME);
    rt_device_open(sensor_temp, RT_DEVICE_FLAG_RDONLY);

    sensor_humi = rt_device_find(SENSOR_HUMI_NAME);
    rt_device_open(sensor_humi, RT_DEVICE_FLAG_RDONLY);

    while (RT_TRUE)
    {
        /* 读取温度数据，并将其填入temp_data 字符串，将温度值转换为浮点型赋值给temp_value */
        rt_device_read(sensor_temp, 0, &temp_data, 1);
        temp_value = (double)temp_data.data.temp / 10;

        if (onenet_mqtt_upload_digit("temperature", temp_value) < 0)
        {
            LOG_E("upload has an error, stop uploading");
            break;
        }
        else
            LOG_D("buffer : {\"temperature\":%d.%d}", (int)temp_value, temp_data.data.temp % 10);

        rt_thread_delay(rt_tick_from_millisecond(1000));
        /* 读取湿度数据，并将其填入humi_data 字符串，将湿度值转换为浮点型赋值给humi_value */
        rt_device_read(sensor_humi, 0, &humi_data, 1);
        humi_value = (double)humi_data.data.humi / 10;

        if (onenet_mqtt_upload_digit("humidity", humi_value) < 0)
        {
            LOG_E("upload has an error, stop uploading");
            break;
        }
        else
            LOG_D("buffer : {\"humidity\":%d.%d}", (int)humi_value, humi_data.data.humi % 10);

        rt_thread_delay(rt_tick_from_millisecond(10 * 1000));
    }

    rt_device_close(sensor_temp);
}

/* onenet mqtt command response callback function */
static void onenet_cmd_rsp_callback(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size)
{
    char res_buf[32] = {0};

    LOG_D("recv data is %.*s\n", recv_size, recv_data);

    /* set LED_R pin mode to output */
    rt_pin_mode(LED_R, PIN_MODE_OUTPUT);

    /* 命令匹配 */
    if (rt_strncmp((const char *)recv_data, "led-on", 6) == 0)
    {
        /* 开灯 */
        rt_pin_write(LED_R, PIN_LOW);

        rt_snprintf(res_buf, sizeof(res_buf), "led is on");

        LOG_D("led is on");
    }
    else if (rt_strncmp((const char *)recv_data, "led-off", 7) == 0)
    {
        /* 关灯 */
        rt_pin_write(LED_R, PIN_HIGH);

        rt_snprintf(res_buf, sizeof(res_buf), "led is off");

        LOG_D("led is off");
    }
    else if (rt_strncmp((const char *)recv_data, "http_ota", 8) == 0)
    {
        /* 开始固件OTA 空中升级过程 */
        if(msh_exec(recv_data, rt_strlen(recv_data)) != 0)
        {
            LOG_E("%s: command not found.", recv_data);
            return;
        }

        rt_snprintf(res_buf, sizeof(res_buf), "Upgrading firmware by ota...");

        LOG_D("Upgrading firmware by ota...");
    }

    /* user have to malloc memory for response data */
    *resp_data = (uint8_t *) ONENET_MALLOC(strlen(res_buf));

    strncpy((char *)*resp_data, (const char *)res_buf, strlen(res_buf));

    *resp_size = strlen(res_buf);
}

/* RT_WLAN_EVT_READY 事件回调函数 */
static void wlan_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    rt_thread_t dp_tid = RT_NULL;

    /* 初始化OneNET 组件包 */
    if(onenet_mqtt_init() != 0)
    {
        LOG_E("OneNET package(V%s) initialize failed.", ONENET_SW_VERSION);
        return;
    }

    /* 等待MQTT 连接建立成功，也可以放到mqtt_online_callback 中创建上传数据点的线程 */
    rt_thread_delay(rt_tick_from_millisecond(6 * 1000));

    /* 创建周期性上传数据点的线程 */
    dp_tid = rt_thread_create("onenet_upload_datapoint",
                           onenet_upload_datapoint_thread,
                           RT_NULL,
                           2 * 1024,
                           RT_THREAD_PRIORITY_MAX / 3 - 1,
                           5);
    if (dp_tid)
        rt_thread_startup(dp_tid);

    /* 注册命令响应回调函数 */
    onenet_set_cmd_rsp_cb(onenet_cmd_rsp_callback);
}

int main(void)
{
    /* 注册 wlan 回调函数 */
    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wlan_ready_handler, RT_NULL);

    /* 初始化 wlan 自动连接配置 */
    wlan_autoconnect_init();
    /* 使能 wlan 自动连接功能 */
    rt_wlan_config_autoreconnect(RT_TRUE);

    /* 打印当前软件版本信息 */
    LOG_I("The current version of APP firmware is %s\n", APP_VERSION);

    return 0;
}