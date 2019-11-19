#include "rtthread.h"
#include "rtdevice.h"
#include "aht10.h"

static void i2c_aht10_sample(void)
{
    float humidity, temperature;
    aht10_device_t aht10_dev;
    const char *i2c_bus_name = "i2c1";
    int count = 0;

    aht10_dev = aht10_init(i2c_bus_name);
    if(aht10_dev == RT_NULL)
        rt_kprintf("The sensor initializes failed.\n");

    while(++count < 10)
    {
        humidity = aht10_read_humidity(aht10_dev);
        rt_kprintf("read aht10 sensor humidity   : %d.%d %%\n", (int)humidity, (int)(humidity * 10) % 10);

        temperature = aht10_read_temperature(aht10_dev);
        if( temperature >= 0 )
            rt_kprintf("read aht10 sensor temperature: %d.%d°C\n", (int)temperature, (int)(temperature * 10) % 10);
        else
            rt_kprintf("read aht10 sensor temperature: %d.%d°C\n", (int)temperature, (int)(-temperature * 10) % 10);
        
        rt_thread_mdelay(1000);
    }
}
MSH_CMD_EXPORT(i2c_aht10_sample, i2c aht10 sample);


#include "sensor_asair_aht10.h"

#define AHT10_I2C_BUS_NAME      "i2c1"

static int rt_hw_aht10_port(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.dev_name = AHT10_I2C_BUS_NAME;
    cfg.intf.type = RT_SENSOR_INTF_I2C;
    cfg.intf.user_data = (void *)AHT10_I2C_ADDR;
    cfg.mode = RT_SENSOR_MODE_POLLING; 
    rt_hw_aht10_init("aht10", &cfg);

    return RT_EOK;
}
INIT_ENV_EXPORT(rt_hw_aht10_port);


#define SENSOR_TEMP_NAME    "temp_aht10"
#define SENSOR_HUMI_NAME    "humi_aht10"


static void sensor_aht10_sample(void)
{
    rt_device_t sensor_temp, sensor_humi;
    struct rt_sensor_data sensor_data;

    sensor_temp = rt_device_find(SENSOR_TEMP_NAME);

    rt_device_open(sensor_temp, RT_DEVICE_FLAG_RDONLY);
    if(rt_device_read(sensor_temp, 0, &sensor_data, 1) == 1)
    {
        rt_kprintf("read aht10 sensor temperature:%3d.%d°C, timestamp:%5d\n", sensor_data.data.temp / 10, sensor_data.data.temp % 10, sensor_data.timestamp);
    }
    rt_device_close(sensor_temp);

    sensor_humi = rt_device_find(SENSOR_HUMI_NAME);

    rt_device_open(sensor_humi, RT_DEVICE_FLAG_RDONLY);
    if(rt_device_read(sensor_humi, 0, &sensor_data, 1) == 1)
    {
        rt_kprintf("read aht10 sensor humidity:%3d.%d%, timestamp:%5d\n", sensor_data.data.humi / 10, sensor_data.data.humi % 10, sensor_data.timestamp);
    }
    rt_device_close(sensor_temp);
}
MSH_CMD_EXPORT_ALIAS(sensor_aht10_sample, sensor_aht10, sensor aht10 sample);

