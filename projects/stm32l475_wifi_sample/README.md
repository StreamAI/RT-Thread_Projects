# AP6181(BCM43362) WiFi模块移植
## 一、WLAN管理框架简介
随着物联网快速发展，越来越多的嵌入式设备上搭载了 WIFI 无线网络设备，为了能够管理 WIFI 网络设备，RT-Thread 引入了 WLAN 设备管理框架。这套框架是 RT-Thread 开发的一套用于管理 WIFI 的中间件：对下连接具体的 WIFI 驱动，控制 WIFI 的连接、断开、扫描等操作；对上承载不同的应用，为应用提供 WIFI 控制、事件、数据导流等操作，为上层应用提供统一的 WIFI 控制接口。

WLAN 框架主要由四个部分组成：Device 驱动接口层，为 WLAN 框架提供统一的调用接口；Manage 管理层为用户提供 WIFI 扫描、连接、断线重连等具体功能；Protocol 协议负责处理 WIFI 上产生的网络数据流，可根据不同的使用场景挂载不同网络协议栈（比如 LWIP ）；Config配置层可以保存 WIFI 配置参数，为用户提供自动连接服务（可从Flash读取曾经连接过的热点配置信息）。WIFI 框架层次图示如下：
![WLAN管理框架](https://img-blog.csdnimg.cn/20200407215108203.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
WLAN管理框架各层功能简介如下：

 - **APP应用层**：是基于 WLAN 框架的具体应用，如 WiFi 相关的 Shell 命令；
 - **Airkiss / Voice 配网层**：提供无线配网和声波配网等功能；
 - **WLAN Manager 管理层**：能够对 WLAN 设备进行控制和管理，具备设置模式、连接热点、断开热点、启动热点、扫描热点等 WLAN 控制相关的功能，还提供断线重连、自动切换热点等管理功能；
 - **WLAN Protocol 协议层**：将数据流递交给具体网络协议进行解析，用户可以指定使用不同的协议进行通信（本文使用LwIP协议）；
 - **WLAN Config 参数管理层**：管理连接成功的热点信息及密码，并写入非易失的存储介质中，可以为用户提供自动连接曾连热点的服务；
 - **WLAN Device 驱动接口层**：对接具体 WLAN 硬件（本文使用AP6181 WIFI 模块），为管理层提供统一的调用接口。

在WLAN Protocol 与 APP 层之间还应包含网络协议层（比如LwIP），甚至是套接字抽象层SAL（包括网络设备无关层netdev），这些并没有表现在上面的WLAN 框架图中，下文介绍LwIP协议栈移植时再详说。

## 二、WLAN Device实现与AP6181 WLAN驱动移植
### 2.1 WLAN Device驱动接口层

 - **WLAN设备数据结构**

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_dev.h

struct rt_wlan_device
{
    struct rt_device device;
    rt_wlan_mode_t mode;
    struct rt_mutex lock;
    struct rt_wlan_dev_event_desc handler_table[RT_WLAN_DEV_EVT_MAX][RT_WLAN_DEV_EVENT_NUM];
    rt_wlan_pormisc_callback_t pormisc_callback;
    const struct rt_wlan_dev_ops *ops;
    rt_uint32_t flags;
    void *prot;
    void *user_data;
};

typedef enum
{
    RT_WLAN_NONE,
    RT_WLAN_STATION,
    RT_WLAN_AP,
    RT_WLAN_MODE_MAX
} rt_wlan_mode_t;

struct rt_wlan_dev_event_desc
{
    rt_wlan_dev_event_handler handler;
    void *parameter;
};

typedef void (*rt_wlan_dev_event_handler)(struct rt_wlan_device *device, rt_wlan_dev_event_t event, struct rt_wlan_buff *buff, void *parameter);

typedef void (*rt_wlan_pormisc_callback_t)(struct rt_wlan_device *device, void *data, int len);

struct rt_wlan_dev_ops
{
    rt_err_t (*wlan_init)(struct rt_wlan_device *wlan);
    rt_err_t (*wlan_mode)(struct rt_wlan_device *wlan, rt_wlan_mode_t mode);
    rt_err_t (*wlan_scan)(struct rt_wlan_device *wlan, struct rt_scan_info *scan_info);
    rt_err_t (*wlan_join)(struct rt_wlan_device *wlan, struct rt_sta_info *sta_info);
    rt_err_t (*wlan_softap)(struct rt_wlan_device *wlan, struct rt_ap_info *ap_info);
    rt_err_t (*wlan_disconnect)(struct rt_wlan_device *wlan);
    rt_err_t (*wlan_ap_stop)(struct rt_wlan_device *wlan);
    rt_err_t (*wlan_ap_deauth)(struct rt_wlan_device *wlan, rt_uint8_t mac[]);
    rt_err_t (*wlan_scan_stop)(struct rt_wlan_device *wlan);
    int (*wlan_get_rssi)(struct rt_wlan_device *wlan);
    rt_err_t (*wlan_set_powersave)(struct rt_wlan_device *wlan, int level);
    int (*wlan_get_powersave)(struct rt_wlan_device *wlan);
    rt_err_t (*wlan_cfg_promisc)(struct rt_wlan_device *wlan, rt_bool_t start);
    rt_err_t (*wlan_cfg_filter)(struct rt_wlan_device *wlan, struct rt_wlan_filter *filter);
    rt_err_t (*wlan_set_channel)(struct rt_wlan_device *wlan, int channel);
    int (*wlan_get_channel)(struct rt_wlan_device *wlan);
    rt_err_t (*wlan_set_country)(struct rt_wlan_device *wlan, rt_country_code_t country_code);
    rt_country_code_t (*wlan_get_country)(struct rt_wlan_device *wlan);
    rt_err_t (*wlan_set_mac)(struct rt_wlan_device *wlan, rt_uint8_t mac[]);
    rt_err_t (*wlan_get_mac)(struct rt_wlan_device *wlan, rt_uint8_t mac[]);
    int (*wlan_recv)(struct rt_wlan_device *wlan, void *buff, int len);
    int (*wlan_send)(struct rt_wlan_device *wlan, void *buff, int len);
};
```

结构体 rt_wlan_device 继承自设备基类 rt_device，自然需要将其注册到 I/O 设备管理层。rt_wlan_device 成员还包括WLAN设备工作模式（Access Point模式还是Station模式）、WLAN设备访问互斥锁、WLAN事件回调函数组、WLAN混杂模式回调函数、需要底层驱动实现并注册的WLAN接口函数集合rt_wlan_dev_ops、WLAN标识位（用于标识工作模式或自动连接状态等）、WLAN设备使用的网络协议栈信息、私有数据等。

 - **WLAN接口函数及设备注册过程**

WLAN设备驱动（这里指的是AP6181 WLAN驱动）需要向WLAN管理框架注册接口函数集合rt_wlan_dev_ops，以便WLAN管理框架对外提供的接口能正常工作，这个函数集合rt_wlan_dev_ops是如何注册到WLAN管理框架的呢？

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_dev.c

rt_err_t rt_wlan_dev_register(struct rt_wlan_device *wlan, const char *name, const struct rt_wlan_dev_ops *ops, rt_uint32_t flag, void *user_data)
{
    rt_err_t err = RT_EOK;

    if ((wlan == RT_NULL) || (name == RT_NULL) || (ops == RT_NULL))
    ......
    rt_memset(wlan, 0, sizeof(struct rt_wlan_device));
    
#ifdef RT_USING_DEVICE_OPS
    wlan->device.ops = &wlan_ops;
#else
    ......
#endif

    wlan->device.user_data  = RT_NULL;
    wlan->device.type = RT_Device_Class_NetIf;

    wlan->ops = ops;
    wlan->user_data  = user_data;

    wlan->flags = flag;
    err = rt_device_register(&wlan->device, name, RT_DEVICE_FLAG_RDWR);

    return err;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops wlan_ops =
{
    _rt_wlan_dev_init,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    _rt_wlan_dev_control
};
#endif
```

从函数rt_wlan_dev_register 的代码可以看出，该函数不仅完成了将函数集合rt_wlan_dev_ops注册到WLAN管理框架的工作（通过参数传递），还完成了将函数集合wlan_ops（通过调用rt_wlan_dev_ops接口实现的rt_device_ops接口）注册到 I/O 设备管理框架的工作，注册的WLAN设备类型为网络接口设备RT_Device_Class_NetIf。

完成WLAN设备向WLAN管理框架和 I/O 设备管理框架的注册后，就可以使用 I/O 设备管理层接口或WLAN Device层提供的接口访问WLAN设备了，我们先看下WLAN设备向 I/O 设备管理层注册的函数集合 wlan_ops 的实现代码：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_dev.c

static rt_err_t _rt_wlan_dev_init(rt_device_t dev)
{
    struct rt_wlan_device *wlan = (struct rt_wlan_device *)dev;
    rt_err_t result = RT_EOK;

    rt_mutex_init(&wlan->lock, "wlan_dev", RT_IPC_FLAG_FIFO);

    if (wlan->ops->wlan_init)
        result = wlan->ops->wlan_init(wlan);
    ......
    return result;
}

static rt_err_t _rt_wlan_dev_control(rt_device_t dev, int cmd, void *args)
{
    struct rt_wlan_device *wlan = (struct rt_wlan_device *)dev;
    rt_err_t err = RT_EOK;

    WLAN_DEV_LOCK(wlan);

    switch (cmd)
    {
    case RT_WLAN_CMD_MODE:
    {
        rt_wlan_mode_t mode = *((rt_wlan_mode_t *)args);
        if (wlan->ops->wlan_mode)
            err = wlan->ops->wlan_mode(wlan, mode);
        break;
    }
    case RT_WLAN_CMD_SCAN:
    {
        struct rt_scan_info *scan_info = args;
        if (wlan->ops->wlan_scan)
            err = wlan->ops->wlan_scan(wlan, scan_info);
        break;
    }
    case RT_WLAN_CMD_JOIN:
    {
        struct rt_sta_info *sta_info = args;
        if (wlan->ops->wlan_join)
            err = wlan->ops->wlan_join(wlan, sta_info);
        break;
    }
    case RT_WLAN_CMD_SOFTAP:
    {
        struct rt_ap_info *ap_info = args;
        if (wlan->ops->wlan_softap)
            err = wlan->ops->wlan_softap(wlan, ap_info);
        break;
    }
    case RT_WLAN_CMD_DISCONNECT:
    {
        if (wlan->ops->wlan_disconnect)
            err = wlan->ops->wlan_disconnect(wlan);
        break;
    }
    case RT_WLAN_CMD_AP_STOP:
    {
        if (wlan->ops->wlan_ap_stop)
            err = wlan->ops->wlan_ap_stop(wlan);
        break;
    }
    case RT_WLAN_CMD_AP_DEAUTH:
    {
        if (wlan->ops->wlan_ap_deauth)
            err = wlan->ops->wlan_ap_deauth(wlan, args);
        break;
    }
    case RT_WLAN_CMD_SCAN_STOP:
    {
        if (wlan->ops->wlan_scan_stop)
            err = wlan->ops->wlan_scan_stop(wlan);
        break;
    }
    case RT_WLAN_CMD_GET_RSSI:
    {
        int *rssi = args;
        if (wlan->ops->wlan_get_rssi)
            *rssi = wlan->ops->wlan_get_rssi(wlan);
        break;
    }
    case RT_WLAN_CMD_SET_POWERSAVE:
    {
        int level = *((int *)args);
        if (wlan->ops->wlan_set_powersave)
            err = wlan->ops->wlan_set_powersave(wlan, level);
        break;
    }
    case RT_WLAN_CMD_GET_POWERSAVE:
    {
        int *level = args;
        if (wlan->ops->wlan_get_powersave)
            *level = wlan->ops->wlan_get_powersave(wlan);
        break;
    }
    case RT_WLAN_CMD_CFG_PROMISC:
    {
        rt_bool_t start = *((rt_bool_t *)args);
        if (wlan->ops->wlan_cfg_promisc)
            err = wlan->ops->wlan_cfg_promisc(wlan, start);
        break;
    }
    case RT_WLAN_CMD_CFG_FILTER:
    {
        struct rt_wlan_filter *filter = args;
        if (wlan->ops->wlan_cfg_filter)
            err = wlan->ops->wlan_cfg_filter(wlan, filter);
        break;
    }
    case RT_WLAN_CMD_SET_CHANNEL:
    {
        int channel = *(int *)args;
        if (wlan->ops->wlan_set_channel)
            err = wlan->ops->wlan_set_channel(wlan, channel);
        break;
    }
    case RT_WLAN_CMD_GET_CHANNEL:
    {
        int *channel = args;
        if (wlan->ops->wlan_get_channel)
            *channel = wlan->ops->wlan_get_channel(wlan);
        break;
    }
    case RT_WLAN_CMD_SET_COUNTRY:
    {
        rt_country_code_t country = *(rt_country_code_t *)args;
        if (wlan->ops->wlan_set_country)
            err = wlan->ops->wlan_set_country(wlan, country);
        break;
    }
    case RT_WLAN_CMD_GET_COUNTRY:
    {
        rt_country_code_t *country = args;
        if (wlan->ops->wlan_get_country)
            *country = wlan->ops->wlan_get_country(wlan);
        break;
    }
    case RT_WLAN_CMD_SET_MAC:
    {
        rt_uint8_t *mac = args;
        if (wlan->ops->wlan_set_mac)
            err = wlan->ops->wlan_set_mac(wlan, mac);
        break;
    }
    case RT_WLAN_CMD_GET_MAC:
    {
        rt_uint8_t *mac = args;
        if (wlan->ops->wlan_get_mac)
            err = wlan->ops->wlan_get_mac(wlan, mac);
        break;
    }
    default:
        break;
    }

    WLAN_DEV_UNLOCK(wlan);

    return err;
}
```

函数集合 wlan_ops 的实现最终都是靠调用WLAN设备驱动提供的函数集合rt_wlan_dev_ops，而且WLAN设备的管理配置主要靠函数rt_device_control 通过发送不同的命令码和参数实现。WLAN Device层提供的接口函数又是通过调用函数集合 wlan_ops 实现的，下面给出WLAN Device层对外提供的接口函数声明：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_dev.h

/* wlan device init */
rt_err_t rt_wlan_dev_init(struct rt_wlan_device *device, rt_wlan_mode_t mode);

/* wlan device station interface */
rt_err_t rt_wlan_dev_connect(struct rt_wlan_device *device, struct rt_wlan_info *info, const char *password, int password_len);
rt_err_t rt_wlan_dev_disconnect(struct rt_wlan_device *device);
int rt_wlan_dev_get_rssi(struct rt_wlan_device *device);

/* wlan device ap interface */
rt_err_t rt_wlan_dev_ap_start(struct rt_wlan_device *device, struct rt_wlan_info *info, const char *password, int password_len);
rt_err_t rt_wlan_dev_ap_stop(struct rt_wlan_device *device);
rt_err_t rt_wlan_dev_ap_deauth(struct rt_wlan_device *device, rt_uint8_t mac[6]);

/* wlan device scan interface */
rt_err_t rt_wlan_dev_scan(struct rt_wlan_device *device, struct rt_wlan_info *info);
rt_err_t rt_wlan_dev_scan_stop(struct rt_wlan_device *device);

/* wlan device mac interface */
rt_err_t rt_wlan_dev_get_mac(struct rt_wlan_device *device, rt_uint8_t mac[6]);
rt_err_t rt_wlan_dev_set_mac(struct rt_wlan_device *device, rt_uint8_t mac[6]);

/* wlan device powersave interface */
rt_err_t rt_wlan_dev_set_powersave(struct rt_wlan_device *device, int level);
int rt_wlan_dev_get_powersave(struct rt_wlan_device *device);

/* wlan device event interface */
rt_err_t rt_wlan_dev_register_event_handler(struct rt_wlan_device *device, rt_wlan_dev_event_t event, rt_wlan_dev_event_handler handler, void *parameter);
rt_err_t rt_wlan_dev_unregister_event_handler(struct rt_wlan_device *device, rt_wlan_dev_event_t event, rt_wlan_dev_event_handler handler);
void rt_wlan_dev_indicate_event_handle(struct rt_wlan_device *device, rt_wlan_dev_event_t event, struct rt_wlan_buff *buff);

/* wlan device promisc interface */
rt_err_t rt_wlan_dev_enter_promisc(struct rt_wlan_device *device);
rt_err_t rt_wlan_dev_exit_promisc(struct rt_wlan_device *device);
rt_err_t rt_wlan_dev_set_promisc_callback(struct rt_wlan_device *device, rt_wlan_pormisc_callback_t callback);
void rt_wlan_dev_promisc_handler(struct rt_wlan_device *device, void *data, int len);

/* wlan device filter interface */
rt_err_t rt_wlan_dev_cfg_filter(struct rt_wlan_device *device, struct rt_wlan_filter *filter);

/* wlan device channel interface */
rt_err_t rt_wlan_dev_set_channel(struct rt_wlan_device *device, int channel);
int rt_wlan_dev_get_channel(struct rt_wlan_device *device);

/* wlan device country interface */
rt_err_t rt_wlan_dev_set_country(struct rt_wlan_device *device, rt_country_code_t country_code);
rt_country_code_t rt_wlan_dev_get_country(struct rt_wlan_device *device);

/* wlan device datat transfer interface */
rt_err_t rt_wlan_dev_report_data(struct rt_wlan_device *device, void *buff, int len);

/* wlan device register interface */
rt_err_t rt_wlan_dev_register(struct rt_wlan_device *wlan, const char *name, 
    const struct rt_wlan_dev_ops *ops, rt_uint32_t flag, void *user_data);
```

WLAN Device层提供的这些接口函数我们虽然可以在应用程序中直接调用，但函数参数有很多结构体类型，在调用这些接口函数前，需要先构造接口函数参数需要的结构体，这就给函数调用带来了不便。WLAN Device层上面的WLAN Manager 层则对这些接口函数进行了再次封装，使用一些全局变量保存必要的信息，简化了参数的构造，我们直接调用WLAN Manager 层提供的接口函数更加方便友好，这些接口函数在下文介绍。

### 2.2 AP6181 WLAN驱动移植
Pandora开发板的程序源码包并没有为我们提供AP6181 WLAN驱动的源码，而是以库文件的形式给出的，所以这里也没法分析AP6181 WLAN驱动的实现原理，只能根据 SDIO 设备管理框架与WLAN 管理框架对WLAN设备驱动的要求推测一些AP6181 WLAN驱动移植时应实现或调用的函数。这里忍不住吐槽一下提供Pandora开发板 [AP6181 WLAN驱动库文件](https://github.com/RT-Thread-packages/wlan-wiced)的同学，起码应该给出一些关于AP6181 WLAN驱动库文件如何使用、需要为其实现哪些接口函数、对外提供哪些接口函数、简单的实现原理之类的说明文档，现在缺少这些信息为WLAN驱动移植和调试带来了很大的不便。

 - **AP6181 WLAN固件配置**

从前篇博客：[SDIO设备对象管理 + AP6181(BCM43362) WiFi模块](https://blog.csdn.net/m0_37621078/article/details/105097567)了解到，AP6181 WIFI 模组内部是需要运行WLAN固件程序的，AP6181 内部可能没有ROM空间，这就需要我们将AP6181 内运行的WLAN固件程序存放到主控端的Flash 空间内。在使用WLAN设备前，由WLAN驱动程序负责将Host 端Flash内存放的WLAN固件读取并传送到AP6181 模组内，以便AP6181 WIFI 模组能正常工作（比如完成WIFI数据帧与以太网数据帧之间的转换）。

这里提醒一点，本文使用的AP6181的WLAN固件与驱动都是从Pandora开发板提供的源码包中获得的，且由于WLAN固件与驱动都是以库文件的形式提供的，对运行环境（比如RT-Thread版本）变更比较敏感，因此最好选择与自己使用的RT-Thread版本一致的 Pandora IOT 源码包。比如我使用的是RT-Thread 4.0.1，正点原子官网给的Pandora IOT 源码包默认的基于RT-Thread 4.0.0开发的，我就需要到GitHub 下载[基于RT-Thread 4.0.1 版本的Pandora IOT 源码包](https://github.com/RT-Thread/IoT_Board/releases)（本文使用的是Release 1.2.0版本）。下文中使用的AP6181 WLAN固件与驱动都是从Pandora IOT Board Release 1.2.0版本源码包拷贝来的。

AP6181 WLAN固件所在路径：

```c
.\IoT_Board\examples\16_iot_wifi_manager\bin\wifi_image_1.0.rbl
```

我们需要先将该WLAN固件放入Flash（Pandora上的W25Q128芯片）的 wifi_image 分区，本文使用的工程文件是基于博客：[FAL分区管理与easyflash变量管理](https://blog.csdn.net/m0_37621078/article/details/102689903)中完成FAL与Easyflash组件移植后的工程文件为基础的。在上面的博客中已经FAL(Flash Abstraction Layer)的实现原理及接口函数，而且在移植FAL组件时配置到分区表也包括wifi_image 分区，这里可以直接该分区存储 AP6181 WLAN 固件镜像文件。

我们如何将AP6181 WLAN固件（wifi_image_1.0.rbl）放到W25Q128 Flash内的wifi_image 分区呢？可以参考下面的文档：

```c
 .\IoT_Board\docs\UM3001-RT-Thread-IoT Board WIFI 模块固件下载手册.pdf
```

> 比较简单的方法是先将WLAN固件放到SD卡如下目录中：
> /SYSTEM/WIFI/wifi_image_1.0.rbl
> \
> 然后将SD卡插入到Pandora开发板的SD卡插槽，将综合例程文件（如下路径）烧录到Pandora开发板中：
> .\IoT_Board\examples\30_iot_board_demo\bin\all.bin
> \
> 综合例程文件烧录完成后，Pandora开发板检测到WLAN固件，会自动执行读取、校验、升级WLAN固件的操作，Pandora开发板的LCD也会显示相应的升级信息（如果wifi_image
> 分区已存在WLAN固件，且与放入SD卡中的WLAN固件版本一致，则不会有相应的加载或升级操作）。

接下来就是AP6181 WLAN驱动负责将存储在W25Q128 Flash wifi_image 分区的WLAN固件读取出来，并通过SDIO总线传输到AP6181 模组内。由于WLAN驱动是以库文件的形式提供的，我们直接从Pandora源码包将WLAN驱动库文件和WLAN驱动移植文件复制到我们的工程中使用，这些文件在Pandora源码包中的路径和复制到我们工程目录的路径如下：

```c
// Pandora IOT Board Release 1.2.0中WLAN驱动库文件和WLAN驱动移植文件路径
.\IoT_Board\libraries\wifi\libwifi_6181_0.2.5_armcm4_gcc.a
.\IoT_Board\libraries\wifi\libwifi_6181_0.2.5_armcm4_iar.a
.\IoT_Board\libraries\wifi\libwifi_6181_0.2.5_armcm4_keil.lib
.\IoT_Board\libraries\wifi\SConscript

.\IoT_Board\drivers\drv_wlan.h
.\IoT_Board\drivers\drv_wlan.c

// WLAN驱动库文件和WLAN驱动移植文件拷贝到我们工程中的目标路径
.\RT-Thread_Projects\libraries\wifi\libwifi_6181_0.2.5_armcm4_gcc.a
.\RT-Thread_Projects\libraries\wifi\libwifi_6181_0.2.5_armcm4_iar.a
.\RT-Thread_Projects\libraries\wifi\libwifi_6181_0.2.5_armcm4_keil.lib
.\RT-Thread_Projects\libraries\wifi\SConscript

.\RT-Thread_Projects\libraries\HAL_Drivers\drv_wlan.h
.\RT-Thread_Projects\libraries\HAL_Drivers\drv_wlan.c
```

WLAN驱动库文件和WLAN驱动移植文件复制到我们工程中后，需要能编译进我们的工程，因此需要修改SConscript文件和SConstruct文件，将我们拷贝过来的文件添加进编译脚本，新增编译代码如下：

```c
// .\RT-Thread_Projects\libraries\HAL_Drivers\SConscript
......
# add wlan driver code
if GetDepend(['BSP_USING_WIFI']):
    src += ['drv_wlan.c']

src += ['drv_common.c']
......

// .\RT-Thread_Projects\projects\stm32l475_wifi_sample\SConstruct
......
# include drivers
objs.extend(SConscript(os.path.join(libraries_path_prefix, 'HAL_Drivers', 'SConscript')))

# include wifi_libraries
objs.extend(SConscript(os.path.join(libraries_path_prefix, 'wifi', 'SConscript')))

# make a building
DoBuilding(TARGET, objs)
```

到这里WLAN驱动库文件和WLAN驱动移植文件就添加到我们的工程中了，接下来看WLAN驱动是如何读取WLAN固件镜像文件的：

```c
// .\RT-Thread_Projects\libraries\HAL_Drivers\drv_wlan.c

#define WIFI_IMAGE_PARTITION_NAME       "wifi_image"
static const struct fal_partition *partition = RT_NULL;

int wiced_platform_resource_size(int resource)
{
    int size = 0;

    /* Download firmware */
    if (resource == 0)
    {
        /* initialize fal */
        fal_init();
        partition = fal_partition_find(WIFI_IMAGE_PARTITION_NAME);
        if (partition == RT_NULL)
            return size;

        if ((rt_ota_init() >= 0) && (rt_ota_part_fw_verify(partition) >= 0))
            size = rt_ota_get_raw_fw_size(partition);
    }
    return size;
}

int wiced_platform_resource_read(int resource, uint32_t offset, void *buffer, uint32_t buffer_size)
{
    int transfer_size = 0;

    if (partition == RT_NULL)
        return 0;

    /* read RF firmware from partition */
    transfer_size = fal_partition_read(partition, offset, buffer, buffer_size);

    return transfer_size;
}
```

我们只需要为WLAN驱动实现两个接口函数：函数wiced_platform_resource_size获得Flash wifi_image分区存储的WLAN固件（wifi_image_1.0.rbl）所占空间的大小size；函数wiced_platform_resource_read则从Flash wifi_image分区读取size大小的数据（实际就是WLAN固件代码），并保存到指针buffer 所指向的内存空间。WLAN固件代码后续的处理（比如通过SDIO总线将其传输到AP6181 模组内）则由WLAN驱动程序完成，不需要我们操心了。

函数wiced_platform_resource_size获取WLAN固件大小的函数rt_ota_get_raw_fw_size（包括函数rt_ota_init与rt_ota_part_fw_verify）均由OTA(Over-The-Air programming)库文件提供，我们还需要将OTA库文件添加进我们的工程中，跟添加WLAN驱动库文件方法一样，如下所示：

```c
// Pandora IOT Board Release 1.2.0中OTA库文件路径
.\IoT_Board\libraries\rt_ota\inc\rt_ota.h
.\IoT_Board\libraries\rt_ota\libs\librt_ota_noalgo_0.1.2_stm32l4_gcc.a
.\IoT_Board\libraries\rt_ota\libs\librt_ota_noalgo_0.1.2_stm32l4_iar.a
.\IoT_Board\libraries\rt_ota\libs\librt_ota_noalgo_0.1.2_stm32l4_keil.lib
.\IoT_Board\libraries\rt_ota\SConscript

// OTA库文件拷贝到我们工程中的目标路径
.\RT-Thread_Projects\libraries\rt_ota\inc\rt_ota.h
.\RT-Thread_Projects\libraries\rt_ota\libs\librt_ota_noalgo_0.1.2_stm32l4_gcc.a
.\RT-Thread_Projects\libraries\rt_ota\libs\librt_ota_noalgo_0.1.2_stm32l4_iar.a
.\RT-Thread_Projects\libraries\rt_ota\libs\librt_ota_noalgo_0.1.2_stm32l4_keil.lib
.\RT-Thread_Projects\libraries\rt_ota\SConscript

// 将拷贝来的rt_ota库文件添加进编译脚本的代码
.\RT-Thread_Projects\projects\stm32l475_wifi_sample\SConstruct
......
# include wifi_libraries
objs.extend(SConscript(os.path.join(libraries_path_prefix, 'wifi', 'SConscript')))

# include ota_libraries
objs.extend(SConscript(os.path.join(libraries_path_prefix, 'rt_ota', 'SConscript')))

# make a building
DoBuilding(TARGET, objs)
```

我们已经将WLAN驱动库文件、WLAN驱动移植文件、OTA库文件都添加进我们的工程中了，但想要将其编译进我们的工程中，还需要配置相应的宏，我们先看看这些组件依赖哪些宏定义：

```c
// .\RT-Thread_Projects\libraries\wifi\SConscript
......
group = DefineGroup('wifi', src, depend = ['RT_USING_WIFI_6181_LIB'], CPPPATH = path, LIBS = LIBS, LIBPATH = LIBPATH)
......
// .\RT-Thread_Projects\libraries\HAL_Drivers\SConscript
......
if GetDepend(['BSP_USING_WIFI']):
    src += ['drv_wlan.c']
......
// .\RT-Thread_Projects\libraries\rt_ota\SConscript
......
group = DefineGroup('rt_ota', src, depend = ['RT_USING_OTA_LIB'], CPPPATH = path, LIBS = libs, LIBPATH = libpath)
......
```
从上面的编译脚本中可以看到三个依赖宏定义需要我们配置，我们在Kconfig中配置这三个宏编译选项的代码如下：

```c
// .\RT-Thread_Projects\projects\stm32l475_wifi_sample\board\Kconfig
......
menu "Onboard Peripheral Drivers"
    ......
    config BSP_USING_WIFI
        bool "Enable WiFi"
        select BSP_USING_SDIO
        select BSP_USING_QSPI_FLASH
        select PKG_USING_FAL
        select RT_USING_WIFI
        select RT_USING_WIFI_6181_LIB
        select RT_USING_OTA_LIB
        select RT_USING_LIBC
        select RT_USING_DFS
        default n

endmenu
......
menu "External Libraries"

    config RT_USING_WIFI_6181_LIB
        bool "Using Wifi(AP6181) Library"
        default n

    config RT_USING_OTA_LIB
        bool "Using RT-Thrad OTA Library"
        default n
    
endmenu

endmenu
```

第一个宏选项BSP_USING_WIFI 依赖项比较多，首先是依赖于BSP_USING_SDIO、BSP_USING_QSPI_FLASH 和 PKG_USING_FAL，前者是使用SDIO外设，后两个是用于管理W25Q128 Flash wifi_image 分区的（用于存储WLAN固件镜像文件）。接下来三个依赖宏RT_USING_WIFI、RT_USING_WIFI_6181_LIB和RT_USING_OTA_LIB 则是使用WLAN驱动库文件和OTA库文件；依赖宏RT_USING_LIBC则是使用C标准库文件（WLAN驱动库文件和OTA库文件内有使用C标准库文件）；依赖宏RT_USING_DFS是使用虚拟文件系统，这个主要是为SD Memory Card作为块设备挂载文件系统存在的。BSP_USING_WIFI 的依赖项还没有列举完全，随着后面介绍会逐渐完善。

后面两个宏选项RT_USING_WIFI_6181_LIB和RT_USING_OTA_LIB比较简单，如果在menuconfig中被选中则相应的宏被定义。

 - **AP6181 WLAN驱动初始化**

WLAN固件与WLAN驱动（包括OTA组件）都已经添加到我们的工程中，接下来看看WLAN驱动初始化过程：

```c
// .\RT-Thread_Projects\libraries\HAL_Drivers\drv_wlan.c

extern int wifi_hw_init(void);
extern void wwd_thread_notify_irq(void);
static rt_uint32_t init_flag = 0;

int rt_hw_wlan_init(void)
{
    if (init_flag == 1)
        return RT_EOK;

#ifdef BSP_USING_WIFI_THREAD_INIT
    rt_thread_t tid = RT_NULL;
    tid = rt_thread_create("wifi_init", wifi_init_thread_entry, RT_NULL, WIFI_INIT_THREAD_STACK_SIZE, WIFI_INIT_THREAD_PRIORITY, 20);
    if (tid)
        rt_thread_startup(tid);
    else
        return -RT_ERROR;
#else
    wifi_init_thread_entry(RT_NULL);
    init_flag = 1;
#endif

    return RT_EOK;
}
#ifdef BSP_USING_WIFI_AUTO_INIT
INIT_APP_EXPORT(rt_hw_wlan_init);
#endif

static void wifi_init_thread_entry(void *parameter)
{
    /* set wifi irq handle, must be initialized first */
    #define PIN_WIFI_IRQ    GET_PIN(C, 5)
    rt_pin_mode(PIN_WIFI_IRQ, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(PIN_WIFI_IRQ, PIN_IRQ_MODE_RISING_FALLING, _wiced_irq_handler, RT_NULL);
    rt_pin_irq_enable(PIN_WIFI_IRQ, PIN_IRQ_ENABLE);

    /* initialize low level wifi(ap6181) library */
    wifi_hw_init();

    /* waiting for sdio bus stability */
    rt_thread_delay(WIFI_INIT_WAIT_TIME);

    /* set wifi work mode */
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);

    init_flag = 1;
}

static void _wiced_irq_handler(void *param)
{
    wwd_thread_notify_irq();
}
```

WLAN初始化函数rt_hw_wlan_init 创建并启动了一个WIFI 初始化线程 wifi_init_thread_entry（是否使用线程取决于宏BSP_USING_WIFI_THREAD_INIT是否被定义），在WIFI 初始化线程中先在PIN_WIFI_IRQ 引脚（也即前篇博客：[SDIO设备对象管理 + AP6181(BCM43362) WiFi模块](https://blog.csdn.net/m0_37621078/article/details/105097567)中介绍的WIFI_INT 引脚）上绑定中断处理函数_wiced_irq_handler（实际绑定的是WLAN驱动库文件中实现的函数wwd_thread_notify_irq），然后执行WIFI 硬件初始化函数 wifi_hw_init （由WLAN驱动库文件实现）完成 AP6181 WIFI 模块的初始化。最后，设置WIFI 工作模式，这里设置为Station 模式，让Pandora 开发板连接周围的WIFI 热点。

这里提醒一点，获得PIN_WIFI_IRQ引脚编号的宏定义是我新增的，Pandora源码包中的头文件drv_gpio.h 定义了开发板上几乎所有的引脚编号，我们的工程使用的通用模板并没有给出太多的引脚定义，因此需要在用到某引脚时再获取引脚编号。

WLAN初始化函数rt_hw_wlan_init 也是可以被自动初始化组件调用的，是否可以自动完成初始化取决于宏BSP_USING_WIFI_AUTO_INIT是否被定义，我们再在宏选项BSP_USING_WIFI 下新增BSP_USING_WIFI_AUTO_INIT与BSP_USING_WIFI_THREAD_INIT的宏配置代码如下：

```c
// .\RT-Thread_Projects\projects\stm32l475_wifi_sample\board\Kconfig

menu "Hardware Drivers Config"
......
menu "Onboard Peripheral Drivers"
	......
    config BSP_USING_WIFI
        bool "Enable WiFi"
        ......
        select RT_USING_DFS
        default n
        if BSP_USING_WIFI
            config BSP_USING_WIFI_THREAD_INIT
                bool "Using Thread Initialize WiFi"
                default n
            config BSP_USING_WIFI_AUTO_INIT
                bool "Using WiFi Automatically Initialization"
                depends on RT_USING_COMPONENTS_INIT
                default y
        endif

endmenu
......
```

WLAN初始化需要一定的时间，WLAN驱动适配层为我们提供了接口函数 rt_hw_wlan_wait_init_done，我们可以在应用中调用该函数等待WLAN设备初始化完成，该函数的实现代码如下：

```c
// .\RT-Thread_Projects\libraries\HAL_Drivers\drv_wlan.c
static rt_uint32_t init_flag = 0;
/**
 * wait milliseconds for wifi low level initialize complete
 * time_ms: timeout in milliseconds
 */ 
int rt_hw_wlan_wait_init_done(rt_uint32_t time_ms)
{
    rt_uint32_t time_cnt = 0;

    /* wait wifi low level initialize complete */
    while (time_cnt <= (time_ms / 100))
    {
        time_cnt++;
        rt_thread_mdelay(100);
        if (rt_hw_wlan_get_initialize_status() == 1)
            break;
    }

    if (time_cnt > (time_ms / 100))
        return -RT_ETIMEOUT;

    return RT_EOK;
}

int rt_hw_wlan_get_initialize_status(void)
{
    return init_flag;	// 1 initialize done;0 not initialize
}
```

## 三、WLAN Protocol实现与LwIP协议栈移植
### 3.1 WLAN Protocol 网络协议层

 - **WLAN协议层数据结构描述**

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_prot.h

struct rt_wlan_prot
{
    char name[RT_WLAN_PROT_NAME_LEN];
    rt_uint32_t id;
    const struct rt_wlan_prot_ops *ops;
};

#define RT_LWAN_ID_PREFIX      (0x5054)

struct rt_wlan_prot_ops
{
    rt_err_t (*prot_recv)(struct rt_wlan_device *wlan, void *buff, int len);
    struct rt_wlan_prot *(*dev_reg_callback)(struct rt_wlan_prot *prot, struct rt_wlan_device *wlan);
    void (*dev_unreg_callback)(struct rt_wlan_prot *prot, struct rt_wlan_device *wlan);
};
```

结构体rt_wlan_prot 包含协议名name、协议ID（由前缀和编号共同构成）、网络协议层应实现并向WLAN管理框架注册的接口函数集合rt_wlan_prot_ops 等成员。

WLAN协议层需要的接口函数rt_wlan_prot_ops 是如何被注册的呢？

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_prot.c

static struct rt_wlan_prot *_prot[RT_WLAN_PROT_MAX];
static struct rt_wlan_prot_event_des prot_event_tab[RT_WLAN_PROT_EVT_MAX][RT_WLAN_PROT_MAX];

rt_err_t rt_wlan_prot_regisetr(struct rt_wlan_prot *prot)
{
    int i;
    rt_uint32_t id;
    static rt_uint8_t num;

    /* Parameter checking */
    if ((prot == RT_NULL) || (prot->ops->prot_recv == RT_NULL) ||
            (prot->ops->dev_reg_callback == RT_NULL))
        return -RT_EINVAL;

    /* save prot */
    for (i = 0; i < RT_WLAN_PROT_MAX; i++)
    {
        if (_prot[i] == RT_NULL)
        {
            id = (RT_LWAN_ID_PREFIX << 16) | num;
            prot->id = id;
            _prot[i] = prot;
            num ++;
            break;
        }
        else if (rt_strcmp(_prot[i]->name, prot->name) == 0)
            break;
    }

    /* is full */
    if (i >= RT_WLAN_PROT_MAX)
        return -RT_ERROR;

    return RT_EOK;
}
```

向WLAN协议层注册网络协议栈 rt_wlan_prot，实际上就是将实现的结构体对象 rt_wlan_prot 赋值给WLAN协议层的全局变量（被static修饰，仅该源文件内有效）_prot[i]，WLAN协议层就可以调用注册来的接口函数 rt_wlan_prot_ops，来实现本层对外提供的接口函数了。

 - **WLAN协议层接口函数**

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_prot.h

/* 网络协议绑定/解绑到WLAN设备 */
rt_err_t rt_wlan_prot_attach(const char *dev_name, const char *prot_name);
rt_err_t rt_wlan_prot_attach_dev(struct rt_wlan_device *wlan, const char *prot_name);
rt_err_t rt_wlan_prot_detach(const char *dev_name);
rt_err_t rt_wlan_prot_detach_dev(struct rt_wlan_device *wlan);
/* rt_wlan_prot协议注册 */
rt_err_t rt_wlan_prot_regisetr(struct rt_wlan_prot *prot);
/* WLAN协议层向设备层发送/接收数据 */
rt_err_t rt_wlan_prot_transfer_dev(struct rt_wlan_device *wlan, void *buff, int len);
rt_err_t rt_wlan_dev_transfer_prot(struct rt_wlan_device *wlan, void *buff, int len);
/* WLAN协议层事件回调函数注册/注销 */
rt_err_t rt_wlan_prot_event_register(struct rt_wlan_prot *prot, rt_wlan_prot_event_t event, rt_wlan_prot_event_handler handler);
rt_err_t rt_wlan_prot_event_unregister(struct rt_wlan_prot *prot, rt_wlan_prot_event_t event);
typedef void (*rt_wlan_prot_event_handler)(struct rt_wlan_prot *port, struct rt_wlan_device *wlan, int event);
/* 执行注册的RT_WLAN_EVT_READY事件回调函数 */
int rt_wlan_prot_ready(struct rt_wlan_device *wlan, struct rt_wlan_buff *buff);
/* 打印所有向WLAN协议层注册的rt_wlan_prot信息 */
void rt_wlan_prot_dump(void);
```

WLAN协议层向设备层发送/接收数据也是通过调用WLAN驱动库文件提供的rt_wlan_dev_ops实现的；WLAN协议层事件回调函数的注册/注销与与WLAN协议结构体rt_wlan_prot 的注册类似，也是将通过参数传入的事件回调函数指针与参数赋值给全局变量prot_event_tab。这里重点看下WLAN网络协议绑定到WLAN设备的过程：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_prot.c

rt_err_t rt_wlan_prot_attach(const char *dev_name, const char *prot_name)
{
    struct rt_wlan_device *wlan;
    wlan = rt_wlan_prot_find_by_name(dev_name);
    ......
    return rt_wlan_prot_attach_dev(wlan, prot_name);
}

rt_err_t rt_wlan_prot_attach_dev(struct rt_wlan_device *wlan, const char *prot_name)
{
    int i = 0;
    struct rt_wlan_prot *prot = wlan->prot;
    rt_wlan_dev_event_t event;
	/* Parameter checking */
    ......
    /* if prot not NULL */
    if (prot != RT_NULL)
        rt_wlan_prot_detach_dev(wlan);

#ifdef RT_WLAN_PROT_LWIP_PBUF_FORCE
    if (rt_strcmp(RT_WLAN_PROT_LWIP, prot_name) != 0)
        return -RT_ERROR;
#endif
    /* find prot */
    for (i = 0; i < RT_WLAN_PROT_MAX; i++)
    {
        if ((_prot[i] != RT_NULL) && (rt_strcmp(_prot[i]->name, prot_name) == 0))
        {
            /* attach prot */
            wlan->prot = _prot[i]->ops->dev_reg_callback(_prot[i], wlan);
            break;
        }
    }

    if (i >= RT_WLAN_PROT_MAX)
        return -RT_ERROR;

    for (event = RT_WLAN_DEV_EVT_INIT_DONE; event < RT_WLAN_DEV_EVT_MAX; event ++)
    {
        if (rt_wlan_dev_register_event_handler(wlan, event, rt_wlan_prot_event_handle, RT_NULL) != RT_EOK)
            LOG_E("prot register event filed:%d", event);
    }
    return RT_EOK;
}
```

函数rt_wlan_prot_attach 主要完成两个操作：一是调用接口函数 rt_wlan_prot_ops->dev_reg_callback完成网络协议适配；二是向WLAN设备层注册事件处理函数 rt_wlan_prot_event_handle，当WLAN设备层有事件发生，WLAN协议层就跟根据发生的事件类型完成相应的事件处理。

 - **WLAN协议层事件状态机**

WLAN运行过程中可能出现的状态或事件比较多，这些状态或事件使用有限状态机模型（可以参考博客：[有限状态机](https://blog.csdn.net/m0_37621078/article/details/90243451)）管理。当WLAN设备时发生了相应的事件（比如设备连接、断开等），能通过执行WLAN协议层向WLAN设备层注册的事件回调函数rt_wlan_prot_event_handle，让WLAN协议层针对发生的事件及时做出相应的处理。下面看看WLAN协议层是如何处理WLAN设备层发生事件的：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_prot.c

struct rt_wlan_prot_event_des
{
    rt_wlan_prot_event_handler handler;
    struct rt_wlan_prot *prot;
};
static struct rt_wlan_prot_event_des prot_event_tab[RT_WLAN_PROT_EVT_MAX][RT_WLAN_PROT_MAX];

static void rt_wlan_prot_event_handle(struct rt_wlan_device *wlan, rt_wlan_dev_event_t event, struct rt_wlan_buff *buff, void *parameter)
{
    int i;
    struct rt_wlan_prot *wlan_prot;
    struct rt_wlan_prot *prot;
    rt_wlan_prot_event_handler handler;
    rt_wlan_prot_event_t prot_event;

    wlan_prot = wlan->prot;
    handler = RT_NULL;
    prot = RT_NULL;
    
    switch (event)
    {
    case RT_WLAN_DEV_EVT_INIT_DONE:
    {
        prot_event = RT_WLAN_PROT_EVT_INIT_DONE;
        break;
    }
    case RT_WLAN_DEV_EVT_CONNECT:
    {
        prot_event = RT_WLAN_PROT_EVT_CONNECT;
        break;
    }
    case RT_WLAN_DEV_EVT_DISCONNECT:
    {
        prot_event = RT_WLAN_PROT_EVT_DISCONNECT;
        break;
    }
    case RT_WLAN_DEV_EVT_AP_START:
    {
        prot_event = RT_WLAN_PROT_EVT_AP_START;
        break;
    }
    case RT_WLAN_DEV_EVT_AP_STOP:
    {
        prot_event = RT_WLAN_PROT_EVT_AP_STOP;
        break;
    }
    case RT_WLAN_DEV_EVT_AP_ASSOCIATED:
    {
        prot_event = RT_WLAN_PROT_EVT_AP_ASSOCIATED;
        break;
    }
    case RT_WLAN_DEV_EVT_AP_DISASSOCIATED:
    {
        prot_event = RT_WLAN_PROT_EVT_AP_DISASSOCIATED;
        break;
    }
    default:
        return;
    }
    
    for (i = 0; i < RT_WLAN_PROT_MAX; i++)
    {
        if ((prot_event_tab[prot_event][i].handler != RT_NULL) &&
                (prot_event_tab[prot_event][i].prot->id == wlan_prot->id))
        {
            handler = prot_event_tab[prot_event][i].handler;
            prot = prot_event_tab[prot_event][i].prot;
            break;
        }
    }
    if (handler != RT_NULL)
        handler(prot, wlan, prot_event);
}
```

函数rt_wlan_prot_event_handle 根据参数中标识的事件类型，去查询注册到WLAN协议层的事件回调函数表prot_event_tab，当查找到标识事件有注册相应的事件回调函数后，便执行对应的事件回调函数，完成WLAN事件的处理。这些事件回调函数一般是由调用者根据需要实现并注册的，WLAN管理框架只是在我们标识的事件发生时自动执行我们设定的处理程序而已。

### 3.2 LwIP协议栈移植

 - **向WLAN协议层注册并适配LwIP协议**

本文使用的网络协议时LwIP，要想让LwIP网络接口层与WLAN协议层能配合工作，首先需要为WLAN协议层实现并注册接口函数集合rt_wlan_prot_ops，下面先看这些接口函数的注册过程：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_lwip.c

static struct rt_wlan_prot_ops ops =
{
    rt_wlan_lwip_protocol_recv,
    rt_wlan_lwip_protocol_register,
    rt_wlan_lwip_protocol_unregister
};

int rt_wlan_lwip_init(void)
{
    static struct rt_wlan_prot prot;
    rt_wlan_prot_event_t event;

    rt_memset(&prot, 0, sizeof(prot));
    rt_strncpy(&prot.name[0], RT_WLAN_PROT_LWIP, RT_WLAN_PROT_NAME_LEN);
    prot.ops = &ops;

    if (rt_wlan_prot_regisetr(&prot) != RT_EOK)
        return -1;

    for (event = RT_WLAN_PROT_EVT_INIT_DONE; event < RT_WLAN_PROT_EVT_MAX; event++)
        rt_wlan_prot_event_register(&prot, event, rt_wlan_lwip_event_handle);

    return 0;
}
INIT_PREV_EXPORT(rt_wlan_lwip_init);
```

函数rt_wlan_lwip_init 主要完成两个操作：一是WLAN协议结构体rt_wlan_prot 的注册（函数rt_wlan_prot_regisetr）；二是向WLAN协议层注册事件处理函数rt_wlan_lwip_event_handle。在函数rt_wlan_prot_event_handle 中最后要调用执行向WLAN协议层注册的事件回调函数，也就是这里注册的事件处理函数rt_wlan_lwip_event_handle。

函数rt_wlan_lwip_init 被自动初始化组件调用执行，不需要我们主动调用。我们先看下LwIP协议是如何适配到WLAN管理框架的：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_lwip.c

struct lwip_prot_des
{
    struct rt_wlan_prot prot;
    struct eth_device eth;
    rt_int8_t connected_flag;
    struct rt_timer timer;
    struct rt_work work;
};

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops wlan_lwip_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    rt_wlan_lwip_protocol_control
};
#endif

static struct rt_wlan_prot *rt_wlan_lwip_protocol_register(struct rt_wlan_prot *prot, struct rt_wlan_device *wlan)
{
    struct eth_device *eth = RT_NULL;
    static rt_uint8_t id = 0;
    char eth_name[4], timer_name[16];
    rt_device_t device = RT_NULL;
    struct lwip_prot_des *lwip_prot;
    ......
    do
    {
        /* find ETH device name */
        eth_name[0] = 'w';
        eth_name[1] = '0' + id++;
        eth_name[2] = '\0';
        device = rt_device_find(eth_name);
    }
    while (device);

    if (id > 9)
        return RT_NULL;

    if (rt_device_open((rt_device_t)wlan, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
        return RT_NULL;

    lwip_prot = rt_malloc(sizeof(struct lwip_prot_des));
    if (lwip_prot == RT_NULL)
    {
        rt_device_close((rt_device_t)wlan);
        return RT_NULL;
    }
    rt_memset(lwip_prot, 0, sizeof(struct lwip_prot_des));

    eth = &lwip_prot->eth;

#ifdef RT_USING_DEVICE_OPS
    eth->parent.ops        = &wlan_lwip_ops;
#else
    ......
#endif

    eth->parent.user_data  = wlan;
    eth->eth_rx     = RT_NULL;
    eth->eth_tx     = rt_wlan_lwip_protocol_send;

    /* register ETH device */
    if (eth_device_init(eth, eth_name) != RT_EOK)
    {
        rt_device_close((rt_device_t)wlan);
        rt_free(lwip_prot);
        return RT_NULL;
    }
    rt_memcpy(&lwip_prot->prot, prot, sizeof(struct rt_wlan_prot));
    if (wlan->mode == RT_WLAN_STATION)
    {
        rt_sprintf(timer_name, "timer_%s", eth_name);
        rt_timer_init(&lwip_prot->timer, timer_name, timer_callback, wlan, rt_tick_from_millisecond(1000),
                      RT_TIMER_FLAG_SOFT_TIMER | RT_TIMER_FLAG_ONE_SHOT);
    }
    netif_set_up(eth->netif);

    return &lwip_prot->prot;
}
```

LwIP协议适配层定义了一个全局结构体 lwip_prot_des ，包含以太网设备对象 eth_device、WLAN协议对象 rt_wlan_prot、网络连接标识connected_flag、定时器对象rt_timer、工作任务对象rt_work 等成员。

LwIP网络接口层处理的是以太网帧数据，这里直接包含了以太网设备对象 eth_device，你可以对比下[ENC28J60以太网卡的数据结构描述](https://blog.csdn.net/m0_37621078/article/details/104836942)，也是包含了eth_device。但对比发现结构体 lwip_prot_des 少了WLAN设备的MAC地址信息，为此实现了一个函数rt_wlan_lwip_protocol_control 用来获取WLAN设备的MAC地址（该函数也仅有这一个功能）。由于以太网设备eth_device 继承自设备基类 rt_device，因此也向 I/O 设备管理框架注册了一个网卡设备（函数eth_device_init，可参考博客：[网络分层结构 + netdev/SAL原理](https://blog.csdn.net/m0_37621078/article/details/104836942)）。

以太网设备 eth_device 需要实现两个接口函数：eth_rx 与 eth_tx，方便LwIP使用网卡设备完成网络数据流的发生/接收。这里注册的eth_tx 接口函数是 rt_wlan_lwip_protocol_send，最终调用的是WLAN驱动库提供的接口rt_wlan_dev_ops->wlan_send；注册的eth_rx 接口则为RT_NULL，LwIP如何通过网卡接收数据呢？

 - **WLAN协议层接收数据的处理**

再回顾下WLAN协议层需要的接口函数集合rt_wlan_prot_ops，除了前面介绍的网络协议注册和注销，还有一个就是数据接收函数rt_wlan_lwip_protocol_recv，该函数被注册到WLAN协议层后，当AP6181 WIFI 芯片接收到数据会调用该函数来处理。ENC28J60以太网卡INT 引脚绑定的中断处理函数 enc28j60_isr 实际调用的是LwIP 网络接口层的函数eth_device_ready，所以LwIP	可以处理ENC28J60网卡的中断信号并从网卡接收数据。

AP6181 WIFI 网卡的WIFI_INT 引脚绑定的中断处理函数 _wiced_irq_handler，也即AP6181 WIFI 网卡的中断信号和数据接收均由WLAN驱动负责，并不由LwIP 网络接口层负责。要移植LwIP 协议栈，需要向WLAN协议层注册接收函数rt_wlan_lwip_protocol_recv，告诉WLAN协议层接收到的网络数据该如何处理，下面看看接收函数rt_wlan_lwip_protocol_recv 是如何处理WLAN网卡接收到数据的：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_lwip.c

static rt_err_t rt_wlan_lwip_protocol_recv(struct rt_wlan_device *wlan, void *buff, int len)
{
    struct eth_device *eth_dev = &((struct lwip_prot_des *)wlan->prot)->eth;
    struct pbuf *p = RT_NULL;
    ......
#ifdef RT_WLAN_PROT_LWIP_PBUF_FORCE
    {
        p = buff;
        if ((eth_dev->netif->input(p, eth_dev->netif)) != ERR_OK)
            return -RT_ERROR;
        
        return RT_EOK;
    }
#else
    {
    	/* alloc pbuf */
        ......
        /*copy data dat -> pbuf*/
        ......
    }
#endif
}
```

AP6181 WIFI 网卡接收到数据后，会调用函数rt_wlan_lwip_protocol_recv 处理接收到的数据，实际上还是调用LwIP网络接口层netif 的input接口（也即函数 tcpip_input）将接收到的数据传递给LwIP 上层进行处理。对 eth_device_init 和 tcpip_input 不熟悉的读者，可以参考博客：[LwIP协议栈移植](https://blog.csdn.net/m0_37621078/article/details/103282134)。

在函数rt_wlan_lwip_protocol_recv中有一个宏定义选项RT_WLAN_PROT_LWIP_PBUF_FORCE，若该宏被定义则直接强制使用LwIP的pbuf 数据包，若该宏未定义，还需要先分配pbuf 对象再拷贝数据，为了尽可能保证效率与性能，我们可以在宏配置选项BSP_USING_WIFI下新增依赖宏RT_WLAN_PROT_LWIP_PBUF_FORCE。

 - **LwIP适配层事件处理**

前面介绍的两个函数rt_wlan_lwip_init 与 rt_wlan_lwip_protocol_register 都还有个尾巴没介绍，分别是LwIP适配层事件处理函数 rt_wlan_lwip_event_handle 和 定时回调函数 timer_callback。先看看LwIP适配层如何处理WLAN协议层的事件：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_lwip.c

static void rt_wlan_lwip_event_handle(struct rt_wlan_prot *port, struct rt_wlan_device *wlan, int event)
{
    struct lwip_prot_des *lwip_prot = (struct lwip_prot_des *)wlan->prot;
    rt_bool_t flag_old;

    flag_old = lwip_prot->connected_flag;

    switch (event)
    {
    case RT_WLAN_PROT_EVT_CONNECT:
    {
        lwip_prot->connected_flag = RT_TRUE;
        break;
    }
    case RT_WLAN_PROT_EVT_DISCONNECT:
    {
        lwip_prot->connected_flag = RT_FALSE;
        break;
    }
    case RT_WLAN_PROT_EVT_AP_START:
    {
        lwip_prot->connected_flag = RT_TRUE;
        break;
    }
    case RT_WLAN_PROT_EVT_AP_STOP:
    {
        lwip_prot->connected_flag = RT_FALSE;
        break;
    }
    case RT_WLAN_PROT_EVT_AP_ASSOCIATED:
        break;
    case RT_WLAN_PROT_EVT_AP_DISASSOCIATED:
        break;
    default :
        break;
    }
    if (flag_old != lwip_prot->connected_flag)
        rt_wlan_workqueue_dowork(netif_set_connected, wlan);
}

static void netif_set_connected(void *parameter)
{
    struct rt_wlan_device *wlan = parameter;
    struct lwip_prot_des *lwip_prot = wlan->prot;
    struct eth_device *eth_dev = &lwip_prot->eth;

    if (lwip_prot->connected_flag)
    {
        if (wlan->mode == RT_WLAN_STATION)
        {
            netifapi_netif_common(eth_dev->netif, netif_set_link_up, NULL);
			......
            rt_timer_start(&lwip_prot->timer);
        }
        else if (wlan->mode == RT_WLAN_AP)
            netifapi_netif_common(eth_dev->netif, netif_set_link_up, NULL);
			......
    }
    else
    {
        if (wlan->mode == RT_WLAN_STATION)
        {
            netifapi_netif_common(eth_dev->netif, netif_set_link_down, NULL);
			......
            rt_timer_stop(&lwip_prot->timer);
        }
        else if (wlan->mode == RT_WLAN_AP)
            netifapi_netif_common(eth_dev->netif, netif_set_link_down, NULL);
    }
}
```

WLAN设备的状态或事件虽多，实际上对于LwIP协议栈来说，主要关心的就两种：连接、断开；LwIP网络接口层做出的处理也是两种：链路层网卡打开、关闭。如果WIFI	Station已连接或AP已启用，LwIP 协议则会打开或启用网络接口，并为其提供网络服务；如果WIFI Station已断开或AP已停止，LwIP 协议则会关闭或禁用网络接口，并停止为其提供网络服务。

接下来再看看向WLAN协议层注册并配置LwIP 协议后，初始化一个定时器，当定时器触发后会执行哪些操作：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_lwip.c

static void timer_callback(void *parameter)
{
    ......
    workqueue = rt_wlan_get_workqueue();
    if (workqueue != RT_NULL)
    {
        level = rt_hw_interrupt_disable();
        rt_work_init(work, netif_is_ready, parameter);
        rt_hw_interrupt_enable(level);
        ......
    }
}

static void netif_is_ready(struct rt_work *work, void *parameter)
{
	......
    if (rt_wlan_prot_ready(wlan, &buff) != 0)
    {
        rt_timer_start(&lwip_prot->timer);
        goto exit;
    }
    ......
}
```

定时器触发后执行函数netif_is_ready，从名字可以看出是LwIP 网络接口配置就绪的函数，其内部调用了WLAN协议层介绍过的接口函数 rt_wlan_prot_ready，前面提到该函数执行注册的RT_WLAN_EVT_READY事件回调函数，这个事件回调函数需要我们自己提前实现并注册，下文再详细介绍。

到这里向WLAN管理框架注册并适配 LwIP 协议栈的工作就完成了，网络设备无关层netdev 和 网络协议无关层 SAL 的移植或适配跟ENC28J60以太网卡中介绍的完全一样，这里就不再赘述了，可以参考博客：[网络分层结构 + netdev/SAL原理](https://blog.csdn.net/m0_37621078/article/details/104836942)。

由于适配了LwIP 协议栈，我们需要在AP6181 WIFI 外设的宏配置选项中新增关于LwIP 的宏依赖项，新增脚本代码如下：

```c
// projects\stm32l475_wifi_sample\board\Kconfig

menu "Hardware Drivers Config"
......
menu "Onboard Peripheral Drivers"
	......
    config BSP_USING_WIFI
        bool "Enable WiFi"
        ......
        select RT_USING_DFS
        select RT_WLAN_PROT_LWIP_PBUF_FORCE
        select RT_USING_LWIP
        ......
endmenu
......
```

## 四、WLAN Config 参数管理与自动连接实现
### 4.1 WLAN Config 参数管理层

 - **WLAN配置信息数据结构描述**

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_cfg.c
struct rt_wlan_cfg_des
{
    rt_uint32_t num;
    struct rt_wlan_cfg_info *cfg_info;
};
static struct rt_wlan_cfg_des *cfg_cache;

// rt-thread-4.0.1\components\drivers\wlan\wlan_cfg.h
struct rt_wlan_cfg_info
{
    struct rt_wlan_info info;
    struct rt_wlan_key key;
};

// rt-thread-4.0.1\components\drivers\wlan\wlan_dev.h
struct rt_wlan_info
{
    /* security type */
    rt_wlan_security_t security;
    /* 2.4G/5G */
    rt_802_11_band_t band;
    /* maximal data rate */
    rt_uint32_t datarate;
    /* radio channel */
    rt_int16_t channel;
    /* signal strength */
    rt_int16_t  rssi;
    /* ssid */
    rt_wlan_ssid_t ssid;
    /* hwaddr */
    rt_uint8_t bssid[RT_WLAN_BSSID_MAX_LENGTH];
    rt_uint8_t hidden;
};

struct rt_wlan_ssid
{
    rt_uint8_t len;
    rt_uint8_t val[RT_WLAN_SSID_MAX_LENGTH + 1];
};
typedef struct rt_wlan_ssid rt_wlan_ssid_t;

struct rt_wlan_key
{
    rt_uint8_t len;
    rt_uint8_t val[RT_WLAN_PASSWORD_MAX_LENGTH + 1];
};
typedef struct rt_wlan_key rt_wlan_key_t;

/* Enumeration of Wi-Fi security modes */
typedef enum
{
    SECURITY_OPEN           = 0,                                                /* Open security                           */
    SECURITY_WEP_PSK        = WEP_ENABLED,                                      /* WEP Security with open authentication   */
    SECURITY_WEP_SHARED     = (WEP_ENABLED | SHARED_ENABLED),                   /* WEP Security with shared authentication */
    SECURITY_WPA_TKIP_PSK   = (WPA_SECURITY  | TKIP_ENABLED),                   /* WPA Security with TKIP                  */
    SECURITY_WPA_AES_PSK    = (WPA_SECURITY  | AES_ENABLED),                    /* WPA Security with AES                   */
    SECURITY_WPA2_AES_PSK   = (WPA2_SECURITY | AES_ENABLED),                    /* WPA2 Security with AES                  */
    SECURITY_WPA2_TKIP_PSK  = (WPA2_SECURITY | TKIP_ENABLED),                   /* WPA2 Security with TKIP                 */
    SECURITY_WPA2_MIXED_PSK = (WPA2_SECURITY | AES_ENABLED | TKIP_ENABLED),     /* WPA2 Security with AES & TKIP           */
    SECURITY_WPS_OPEN       = WPS_ENABLED,                                      /* WPS with open security                  */
    SECURITY_WPS_SECURE     = (WPS_ENABLED | AES_ENABLED),                      /* WPS with AES security                   */
    SECURITY_UNKNOWN        = -1,                                               /* May be returned by scan function if security is unknown.
                                                                                    Do not pass this to the join function! */
} rt_wlan_security_t;

typedef enum
{
    RT_802_11_BAND_5GHZ  =  0,             /* Denotes 5GHz radio band   */
    RT_802_11_BAND_2_4GHZ =  1,            /* Denotes 2.4GHz radio band */
    RT_802_11_BAND_UNKNOWN = 0x7fffffff,   /* unknown */
} rt_802_11_band_t;
```

配置信息结构体rt_wlan_cfg_info 包含的配置项有：WIFI 安全模式、WIFI 频段、最大数据传输速率、通信信道、信号强度、WIFI热点名SSID、WIFI热点密码key、WIFI热点MAC地址、SSID是否隐藏标识等。

 - **WLAN配置管理层接口函数**

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_cfg.h
/* WLAN配置管理层初始化 */
void rt_wlan_cfg_init(void);
/* 向WLAN配置管理层注册接口函数 */
void rt_wlan_cfg_set_ops(const struct rt_wlan_cfg_ops *ops);
/* 获得WLAN配置的WIFI热点信息数量 */
int rt_wlan_cfg_get_num(void);
/* 从缓存中读取所有的WIFI热点配置信息 */
int rt_wlan_cfg_read(struct rt_wlan_cfg_info *cfg_info, int num);
/* 从缓存中读取指定的WIFI热点配置信息 */
int rt_wlan_cfg_read_index(struct rt_wlan_cfg_info *cfg_info, int index);
/* 从Flash中读取存储的WIFI配置信息到缓存中 */
rt_err_t rt_wlan_cfg_cache_refresh(void);
/* 将WIFI配置信息保存到缓存和Flash闪存中 */
rt_err_t rt_wlan_cfg_save(struct rt_wlan_cfg_info *cfg_info);
/* 将缓存中的WIFI配置信息保存到Flash闪存中 */
rt_err_t rt_wlan_cfg_cache_save(void);
/* 从缓存中删除指定的WIFI热点配置信息 */
int rt_wlan_cfg_delete_index(int index);
/* 从缓存中删除所有的WIFI热点配置信息 */
void rt_wlan_cfg_delete_all(void);
/* 打印缓存中所有的WIFI热点配置信息 */
void rt_wlan_cfg_dump(void);
```

在缓存或内存中读取、保存或写入、删除数据（配置信息）比较容易理解，重点是将缓存中的数据保存到Flash闪存，或从Flash闪存中存储的配置信息读取到缓存中，需要向WLAN配置管理层提供访问Flash的接口函数。上面介绍过的函数 rt_wlan_cfg_set_ops 便可以向WLAN Config层注册接口函数集合 rt_wlan_cfg_ops，下面看WLAN Config层需要哪些接口函数，又是如何注册的：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_cfg.h

struct rt_wlan_cfg_ops
{
    int (*read_cfg)(void *buff, int len);
    int (*get_len)(void);
    int (*write_cfg)(void *buff, int len);
};

// rt-thread-4.0.1\components\drivers\wlan\wlan_cfg.c

struct rt_wlan_cfg_des
{
    rt_uint32_t num;
    struct rt_wlan_cfg_info *cfg_info;
};
static struct rt_wlan_cfg_des *cfg_cache;
static const struct rt_wlan_cfg_ops *cfg_ops;

void rt_wlan_cfg_set_ops(const struct rt_wlan_cfg_ops *ops)
{
    rt_wlan_cfg_init();

    WLAN_CFG_LOCK();
    /* save ops pointer */
    cfg_ops = ops;
    WLAN_CFG_UNLOCK();
}
```

需要向WLAN Config层注册的接口函数rt_wlan_cfg_ops 包含读取数据、获取数据长度、写入数据这三个成员，注册函数 rt_wlan_cfg_set_ops 则是将参数传入的接口函数集合赋值给WLAN Config层的全局变量cfg_cache。

### 4.2 rt_wlan_cfg_ops 实现与注册
我们在博客：[FAL分区管理与easyflash变量管理](https://blog.csdn.net/m0_37621078/article/details/102689903)中，不仅介绍了FAL分区管理（比如WLAN固件库文件存储的wifi_image 分区），还介绍了Easyflash变量管理。博客中介绍的变量管理主要用于管理环境变量，当然也可以用来管理这里的WLAN配置信息，所以我们可以使用Easyflash组件提供的接口函数来实现rt_wlan_cfg_ops。

我们对AP6181 WLAN驱动处理WIFI配置信息的数据格式要求并不了解，仍然从Pandora源码包中复制WLAN Config移植文件：

```c
// Pandora IOT Board Release 1.2.0中WLAN Config移植文件路径
.\IoT_Board\examples\16_iot_wifi_manager\ports\wifi\wifi_config.h
.\IoT_Board\examples\16_iot_wifi_manager\ports\wifi\wifi_config.c
.\IoT_Board\examples\16_iot_wifi_manager\ports\wifi\SConscript

// WLAN Config移植文件拷贝到我们工程中的目标路径
.\RT-Thread_Projects\projects\stm32l475_wifi_sample\ports\wifi\wifi_config.h
.\RT-Thread_Projects\projects\stm32l475_wifi_sample\ports\wifi\wifi_config.c
.\RT-Thread_Projects\projects\stm32l475_wifi_sample\ports\wifi\SConscript
```

由于我们工程的ports目录下有一个SConscript 脚本文件，可以将ports目录下的所有子目录内的SConscript 文件都添加到工程中，这里就不需要额外新增编译控制脚本代码了。

从wifi_config.c 代码中可以看出，WLAN配置信息的读取和写入涉及到了Base64编解码，Base64可以看作是一种数据加解密算法。对WLAN配置信息进行加密处理，而不是直接将ASCII明文存储到Flash中，可以防止恶意者泄露我们的WLAN配置信息，保障网络通信安全。当然你也可以采用其它的加解密算法，甚至不采用加解密算法。

我们先不关心WLAN配置信息的加解密算法，看看接口函数集合 rt_wlan_cfg_ops  是如何实现并注册的：

```c
// projects\stm32l475_wifi_sample\ports\wifi\wifi_config.c

static int read_cfg(void *buff, int len)
{
    char *wlan_cfg_info = RT_NULL;

    wlan_cfg_info = ef_get_env("wlan_cfg_info");
    if (wlan_cfg_info != RT_NULL)
    {
        str_base64_decode(wlan_cfg_info, rt_strlen(wlan_cfg_info), buff);
        return len;
    }
    else
        return 0;
}

static int get_len(void)
{
    int len;
    char *wlan_cfg_len = RT_NULL;

    wlan_cfg_len = ef_get_env("wlan_cfg_len");
    if (wlan_cfg_len == RT_NULL)
        len = 0;
    else
        len = atoi(wlan_cfg_len);

    return len;
}

static int write_cfg(void *buff, int len)
{
    char wlan_cfg_len[12] = {0};
    char *base64_buf = RT_NULL;
    
    base64_buf = rt_malloc(len * 4 / 3 + 4); /* 3-byte blocks to 4-byte, and the end. */
    if (base64_buf == RT_NULL)
        return -RT_ENOMEM;
    rt_memset(base64_buf, 0, len);
    
    /* interger to string */
    sprintf(wlan_cfg_len, "%d", len);
    /* set and store the wlan config lengths to Env */
    ef_set_env("wlan_cfg_len", wlan_cfg_len);
    str_base64_encode_len(buff, base64_buf, len);
    /* set and store the wlan config information to Env */
    ef_set_env("wlan_cfg_info", base64_buf);
    ef_save_env();
    rt_free(base64_buf);

    return len;
}

static const struct rt_wlan_cfg_ops ops =
{
    read_cfg,
    get_len,
    write_cfg
};

void wlan_autoconnect_init(void)
{
    fal_init();
    easyflash_init();

    rt_wlan_cfg_set_ops(&ops);
    rt_wlan_cfg_cache_refresh();
}
```

在函数wlan_autoconnect_init 中，先执行fal_init 与 easyflash_init  完成FAL组件与Easyflash组件的初始化，接着通过前面介绍的函数rt_wlan_cfg_set_ops 将这里实现的接口函数集rt_wlan_cfg_ops 注册到WLAN Config层，最后调用函数rt_wlan_cfg_cache_refresh 将Flash中存储的WLAN配置信息读取到缓存中，供WLAN管理框架使用。

函数wlan_autoconnect_init 并没有被自动初始化组件调用，我们可以在这里添加代码，将该函数的调用执行交给RT-Thread的自动初始化组件，也可以在应用中主动调用函数wlan_autoconnect_init ，本文就保持默认，在应用中根据需要主动调用吧。

这里又使用Easyflash组件，我们需要再往宏配置选项BSP_USING_WIFI 下新增依赖宏PKG_USING_EASYFLASH。WLAN管理框架中的Device层、Protocol层和Config层的移植和适配已经完成，依赖的组件已经添加进工程；WLAN Manager 层并不依赖其它组件，主要是为我们提供更方便友好的WLAN管理接口，WLAN Airkiss / Voice配网层算是可选功能，本文暂不介绍，所以宏配置选项BSP_USING_WIFI 下的依赖宏基本全部确定了，下面给出该部分的完整版配置代码：

```c
// .\RT-Thread_Projects\projects\stm32l475_wifi_sample\board\Kconfig

menu "Hardware Drivers Config"
......
menu "Onboard Peripheral Drivers"
	......
    config BSP_USING_WIFI
        bool "Enable WiFi"
        select BSP_USING_SDIO
        select BSP_USING_QSPI_FLASH
        select PKG_USING_FAL
        select RT_USING_WIFI
        select RT_USING_WIFI_6181_LIB
        select RT_USING_OTA_LIB
        select RT_USING_LIBC
        select RT_USING_DFS
        select RT_WLAN_PROT_LWIP_PBUF_FORCE
        select RT_USING_LWIP
        select PKG_USING_EASYFLASH
        default n
        if BSP_USING_WIFI
            config BSP_USING_WIFI_THREAD_INIT
                bool "Using Thread Initialize WiFi"
                default n
            config BSP_USING_WIFI_AUTO_INIT
                bool "Using WiFi Automatically Initialization"
                depends on RT_USING_COMPONENTS_INIT
                default y
        endif

endmenu
```

## 五、WLAN Manager 实现原理
WLAN管理层直接向用户提供WLAN设备的访问接口，能够对 WLAN 设备进行控制和管理，该层接口函数的实现多数都是对WLAN Device设备层接口函数的再封装，但比WLAN Device层提供的接口函数更方便友好。

 - **WLAN管理结构描述**

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_mgnt.c
struct rt_wlan_mgnt_des
{
    struct rt_wlan_device *device;
    struct rt_wlan_info info;
    struct rt_wlan_key key;
    rt_uint8_t state;
    rt_uint8_t flags;
};

static struct rt_wlan_mgnt_des _sta_mgnt;
static struct rt_wlan_mgnt_des _ap_mgnt;
static struct rt_wlan_scan_result scan_result;

// rt-thread-4.0.1\components\drivers\wlan\wlan_mgnt.h
struct rt_wlan_scan_result
{
    rt_int32_t num;
    struct rt_wlan_info *info;
};

/*state fot station*/
#define RT_WLAN_STATE_CONNECT     (1UL << 0)
#define RT_WLAN_STATE_CONNECTING  (1UL << 1)
#define RT_WLAN_STATE_READY       (1UL << 2)
#define RT_WLAN_STATE_POWERSAVE   (1UL << 3)

/*flags fot station*/
#define RT_WLAN_STATE_AUTOEN      (1UL << 0)
/*state fot ap*/
#define RT_WLAN_STATE_ACTIVE      (1UL << 0)
```

WLAN管理结构体rt_wlan_mgnt_des 包含WLAN设备对象指针 *device、WLAN信息结构体 info、WIFI热点密码key、WLAN所处的状态state（CONNECT / CONNECTING / READY / POWERSAVE）、WLAN标识位（Station AUTOEN / AP ACTIVE）等成员，并且Station和AP各创建一个WLAN管理对象（全局变量 _sta_mgnt、_ap_mgnt）。

WLAN信息结构体rt_wlan_info 和WIFI 热点密码结构体 rt_wlan_key，在前面WLAN Config层介绍WLAN配置信息结构体rt_wlan_cfg_info 时都简单介绍过了。WLAN 在连接前扫描周围的热点信息也是经常遇到的场景，因此WLAN管理框架为便于保存扫描出的周围WIFI 热点信息，提供了一个结构体 rt_wlan_scan_result，用来保存每个WIFI 热点信息结构体指针 *info 和扫描到的WIFI 热点数量num。

 - **WLAN管理层接口函数**

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_mgnt.h

/* wifi init interface */
int rt_wlan_init(void);
rt_err_t rt_wlan_set_mode(const char *dev_name, rt_wlan_mode_t mode);
rt_wlan_mode_t rt_wlan_get_mode(const char *dev_name);

/* wifi station mode interface */
rt_err_t rt_wlan_connect(const char *ssid, const char *password);
rt_err_t rt_wlan_connect_adv(struct rt_wlan_info *info, const char *password);
rt_err_t rt_wlan_disconnect(void);
rt_bool_t rt_wlan_is_connected(void);
rt_bool_t rt_wlan_is_ready(void);
rt_err_t rt_wlan_set_mac(rt_uint8_t *mac);
rt_err_t rt_wlan_get_mac(rt_uint8_t *mac);
rt_err_t rt_wlan_get_info(struct rt_wlan_info *info);
int rt_wlan_get_rssi(void);

/* wifi ap mode interface */
rt_err_t rt_wlan_start_ap(const char *ssid, const char *password);
rt_err_t rt_wlan_start_ap_adv(struct rt_wlan_info *info, const char *password);
rt_bool_t rt_wlan_ap_is_active(void);
rt_err_t rt_wlan_ap_stop(void);
rt_err_t rt_wlan_ap_get_info(struct rt_wlan_info *info);
int rt_wlan_ap_get_sta_num(void);
int rt_wlan_ap_get_sta_info(struct rt_wlan_info *info, int num);
rt_err_t rt_wlan_ap_deauth_sta(rt_uint8_t *mac);
rt_err_t rt_wlan_ap_set_country(rt_country_code_t country_code);
rt_country_code_t rt_wlan_ap_get_country(void);

/* wifi scan interface */
rt_err_t rt_wlan_scan(void);
struct rt_wlan_scan_result *rt_wlan_scan_sync(void);
struct rt_wlan_scan_result *rt_wlan_scan_with_info(struct rt_wlan_info *info);
int rt_wlan_scan_get_info_num(void);
int rt_wlan_scan_get_info(struct rt_wlan_info *info, int num);
struct rt_wlan_scan_result *rt_wlan_scan_get_result(void);
void rt_wlan_scan_result_clean(void);
int rt_wlan_scan_find_cache(struct rt_wlan_info *info, struct rt_wlan_info *out_info, int num);
rt_bool_t rt_wlan_find_best_by_cache(const char *ssid, struct rt_wlan_info *info);

/* wifi auto connect interface */
void rt_wlan_config_autoreconnect(rt_bool_t enable);
rt_bool_t rt_wlan_get_autoreconnect_mode(void);

/* wifi power management interface */
rt_err_t rt_wlan_set_powersave(int level);
int rt_wlan_get_powersave(void);

/* wifi event management interface */
rt_err_t rt_wlan_register_event_handler(rt_wlan_event_t event, rt_wlan_event_handler handler, void *parameter);
rt_err_t rt_wlan_unregister_event_handler(rt_wlan_event_t event);

/* wifi management lock interface */
void rt_wlan_mgnt_lock(void);
void rt_wlan_mgnt_unlock(void);
```

从上面WLAN管理层的接口函数声明与前面介绍的WLAN设备接口层的接口函数声明对比可以看出，WLAN管理层的接口函数参数比较简单友好，没有那么多结构体需要构造，而且参数个数也减少了，这是如何做到的呢？

WLAN管理层定义了不少全局变量（虽被static 修饰，在WLAN管理层内部也即本文档内不受限制），将WLAN管理层常用到的信息（比如管理结构体rt_wlan_mgnt_des）保存到全局变量中，WLAN管理层内部函数就可以直接使用这些全局变量存储的信息，而不需要再通过参数传入。这些全局变量的初始化可以在WLAN管理层初始化函数中看到：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_mgnt.c

int rt_wlan_init(void)
{
    static rt_int8_t _init_flag = 0;

    /* Execute only once */
    if (_init_flag == 0)
    {
        rt_memset(&_sta_mgnt, 0, sizeof(struct rt_wlan_mgnt_des));
        rt_memset(&_ap_mgnt, 0, sizeof(struct rt_wlan_mgnt_des));
        rt_memset(&scan_result, 0, sizeof(struct rt_wlan_scan_result));
        rt_memset(&sta_info, 0, sizeof(struct rt_wlan_sta_des));
        rt_mutex_init(&mgnt_mutex, "mgnt", RT_IPC_FLAG_FIFO);
        rt_mutex_init(&scan_result_mutex, "scan", RT_IPC_FLAG_FIFO);
        rt_mutex_init(&sta_info_mutex, "sta", RT_IPC_FLAG_FIFO);
        rt_mutex_init(&complete_mutex, "complete", RT_IPC_FLAG_FIFO);
        rt_timer_init(&reconnect_time, "wifi_tim", rt_wlan_cyclic_check, RT_NULL, DISCONNECT_RESPONSE_TICK, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
        rt_timer_start(&reconnect_time);
        _init_flag = 1;
    }
    return 0;
}
INIT_PREV_EXPORT(rt_wlan_init);
```

函数rt_wlan_init 被自动初始化组件调用，除了初始化WLAN管理层用到的全局变量和互斥量，还初始化并启动了一个重连定时器，当定时器超时触发会执行注册到的超时回调函数rt_wlan_cyclic_check，这个函数做了哪些操作呢？

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_mgnt.c

static void rt_wlan_cyclic_check(void *parameter)
{
    struct rt_workqueue *workqueue;
    static struct rt_work work;
    rt_base_t level;

    if ((_is_do_connect() == RT_TRUE) && (work.work_func == RT_NULL))
    {
        workqueue = rt_wlan_get_workqueue();
        if (workqueue != RT_NULL)
        {
            level = rt_hw_interrupt_disable();
            rt_work_init(&work, rt_wlan_auto_connect_run, RT_NULL);
            rt_hw_interrupt_enable(level);
            if (rt_workqueue_dowork(workqueue, &work) != RT_EOK)
            ......
        }
    }
}

static void rt_wlan_auto_connect_run(struct rt_work *work, void *parameter)
{
    static rt_uint32_t id = 0;
    struct rt_wlan_cfg_info cfg_info;
    char *password = RT_NULL;
    rt_base_t level;
    ......
    /* auto connect status is disable or wifi is connect or connecting, exit */
    ......
    /* Read the next configuration */
    rt_memset(&cfg_info, 0, sizeof(struct rt_wlan_cfg_info));
    if (rt_wlan_cfg_read_index(&cfg_info, id ++) == 0)
    {
        id = 0;
        goto exit;
    }
    
    if (id >= rt_wlan_cfg_get_num()) id = 0;

    if ((cfg_info.key.len > 0) && (cfg_info.key.len < RT_WLAN_PASSWORD_MAX_LENGTH))
    {
        cfg_info.key.val[cfg_info.key.len] = '\0';
        password = (char *)(&cfg_info.key.val[0]);
    }
    rt_wlan_connect_adv(&cfg_info.info, password);
exit:
    ......
}
```

重连定时器reconnect_time 的回调函数rt_wlan_cyclic_check 实际相当于调用执行函数rt_wlan_auto_connect_run，该函数从缓存中读取WLAN配置信息，并逐个尝试使用读取的WIFI 热点配置信息连接周围的热点（通过函数rt_wlan_connect_adv），如果连接失败会尝试下一个WIFI 热点配置信息（重连定时器reconnect_time 为周期定时器）。

前面介绍过的WLAN驱动移植适配层（源文件drv_wlan.c）初始化函数 rt_hw_wlan_init 中，在完成WLAN设备驱动初始化（函数 wifi_hw_init）后，会调用WLAN管理层的函数 rt_wlan_set_mode，该函数除了设置WLAN工作模式还执行了什么操作呢？

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_mgnt.c

rt_err_t rt_wlan_set_mode(const char *dev_name, rt_wlan_mode_t mode)
{
    rt_device_t device = RT_NULL;
    rt_err_t err;
    rt_int8_t up_event_flag = 0;
    rt_wlan_dev_event_handler handler = RT_NULL;
    ......
    /* find device */
    device = rt_device_find(dev_name);
    ......
    /* device == sta and change to ap, should deinit; device == ap and change to sta, should deinit */
    if (((mode == RT_WLAN_STATION) && (RT_WLAN_DEVICE(device) == AP_DEVICE())) ||
            ((mode == RT_WLAN_AP) && (RT_WLAN_DEVICE(device) == STA_DEVICE())))
    {
        err = rt_wlan_set_mode(dev_name, RT_WLAN_NONE);
        ......
    }
    /* init device */
    err = rt_wlan_dev_init(RT_WLAN_DEVICE(device), mode);
    ......
    /* the mode is none */
    if (mode == RT_WLAN_NONE)
    {
        up_event_flag = 1;
        handler = RT_NULL;
		......
    }
    /* save sta device */
    else if (mode == RT_WLAN_STATION)
    {
        up_event_flag = 1;
        handler = rt_wlan_event_dispatch;
        _sta_mgnt.device = RT_WLAN_DEVICE(device);
    }
    /* save ap device */
    else if (mode == RT_WLAN_AP)
    {
        up_event_flag = 1;
        handler = rt_wlan_event_dispatch;
        _ap_mgnt.device = RT_WLAN_DEVICE(device);
    }

    /* update dev event handle */
    if (up_event_flag == 1)
    {
        rt_wlan_dev_event_t event;
        for (event = RT_WLAN_DEV_EVT_INIT_DONE; event < RT_WLAN_DEV_EVT_MAX; event++)
        {
            if (handler)
                rt_wlan_dev_register_event_handler(RT_WLAN_DEVICE(device), event, handler, RT_NULL);
            else
                rt_wlan_dev_unregister_event_handler(RT_WLAN_DEVICE(device), event, handler);
        }
    }
    MGNT_UNLOCK();

    /* Mount protocol */
#ifdef RT_WLAN_DEFAULT_PROT
    rt_wlan_prot_attach(dev_name, RT_WLAN_DEFAULT_PROT);
#endif
    return err;
}
```

函数rt_wlan_set_mode 不仅完成WLAN工作模式的配置（函数 rt_wlan_dev_init 中通过调用rt_wlan_dev_ops->wlan_mode 实现），还完成了WLAN设备事件回调函数 rt_wlan_event_dispatch 的注册。当WLAN Device有事件发生时，会调用WLAN管理层注册的事件处理函数rt_wlan_event_dispatch 完成对相应事件的处理。

函数rt_wlan_set_mode最后调用了WLAN协议层介绍过的协议绑附适配函数rt_wlan_prot_attach，在该函数中也是向WLAN设备接口层注册了事件回调函数 rt_wlan_prot_event_handle。当WLAN Device有事件发生时，也会调用WLAN协议层注册的事件处理函数rt_wlan_prot_attach（最终是调用LwIP适配层向WLAN协议层注册的事件处理函数rt_wlan_lwip_event_handle）完成对相应事件的处理。

WLAN管理层与WLAN协议层都向WLAN设备接口层注册了事件处理回调函数，二者会发生冲突吗？回头看WLAN设备接口层对事件回调函数列表的定义，向WLAN设备接口层注册事件回调函数以及该层执行注册的回调函数的过程如下：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_dev.h

#define RT_WLAN_DEV_EVENT_NUM  (2)   /* EVENT GROUP MAX NUM */

struct rt_wlan_device
{
    ......
    struct rt_wlan_dev_event_desc handler_table[RT_WLAN_DEV_EVT_MAX][RT_WLAN_DEV_EVENT_NUM];
    ......
};

// rt-thread-4.0.1\components\drivers\wlan\wlan_dev.c

rt_err_t rt_wlan_dev_register_event_handler(struct rt_wlan_device *device, rt_wlan_dev_event_t event, rt_wlan_dev_event_handler handler, void *parameter)
{
    ......
    for (i = 0; i < RT_WLAN_DEV_EVENT_NUM; i++)
    {
        if (device->handler_table[event][i].handler == RT_NULL)
        {
            device->handler_table[event][i].handler = handler;
            device->handler_table[event][i].parameter = parameter;
            rt_hw_interrupt_enable(level);
            return RT_EOK;
        }
    }
    ......
}

void rt_wlan_dev_indicate_event_handle(struct rt_wlan_device *device, rt_wlan_dev_event_t event, struct rt_wlan_buff *buff)
{
    void *parameter[RT_WLAN_DEV_EVENT_NUM];
    rt_wlan_dev_event_handler handler[RT_WLAN_DEV_EVENT_NUM];
    int i;
    rt_base_t level;
    ......
    /* get callback handle */
    level = rt_hw_interrupt_disable();
    for (i = 0; i < RT_WLAN_DEV_EVENT_NUM; i++)
    {
        handler[i] = device->handler_table[event][i].handler;
        parameter[i] = device->handler_table[event][i].parameter;
    }
    rt_hw_interrupt_enable(level);

    /* run callback */
    for (i = 0; i < RT_WLAN_DEV_EVENT_NUM; i++)
    {
        if (handler[i] != RT_NULL)
            handler[i](device, event, buff, parameter[i]);
    }
}
```

WLAN设备接口层定义的事件回调函数表是一个二维数组，第二个维度存储的是组数，按照注册的先后顺序，WLAN管理层注册的事件回调函数是第一组，WLAN协议层注册的事件回调函数是第二组。当WLAN设备层有事件发生时，注册的所有事件回调函数都会被依序调用执行。

 - **WLAN管理层事件状态机**

下面看WLAN管理层的事件调度处理函数实现过程：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_mgnt.c

static void rt_wlan_event_dispatch(struct rt_wlan_device *device, rt_wlan_dev_event_t event, struct rt_wlan_buff *buff, void *parameter)
{
    rt_base_t level;
    void *user_parameter;
    rt_wlan_event_handler handler = RT_NULL;
    rt_err_t err = RT_NULL;
    rt_wlan_event_t user_event = RT_WLAN_EVT_MAX;
    int i;
    struct rt_wlan_buff user_buff = { 0 };

    if (buff)
        user_buff = *buff;
    /* Event Handle */
    switch (event)
    {
    case RT_WLAN_DEV_EVT_CONNECT:
    {
        _sta_mgnt.state |= RT_WLAN_STATE_CONNECT;
        _sta_mgnt.state &= ~RT_WLAN_STATE_CONNECTING;
        user_event = RT_WLAN_EVT_STA_CONNECTED;
        TIME_STOP();
        rt_wlan_send_msg(event, RT_NULL, 0);
        user_buff.data = &_sta_mgnt.info;
        user_buff.len = sizeof(struct rt_wlan_info);
        break;
    }
    case RT_WLAN_DEV_EVT_CONNECT_FAIL:
    {
        _sta_mgnt.state &= ~RT_WLAN_STATE_CONNECT;
        _sta_mgnt.state &= ~RT_WLAN_STATE_CONNECTING;
        _sta_mgnt.state &= ~RT_WLAN_STATE_READY;
        user_event = RT_WLAN_EVT_STA_CONNECTED_FAIL;
        user_buff.data = &_sta_mgnt.info;
        user_buff.len = sizeof(struct rt_wlan_info);
        TIME_START();
        break;
    }
    case RT_WLAN_DEV_EVT_DISCONNECT:
    {
        _sta_mgnt.state &= ~RT_WLAN_STATE_CONNECT;
        _sta_mgnt.state &= ~RT_WLAN_STATE_READY;
        user_event = RT_WLAN_EVT_STA_DISCONNECTED;
        user_buff.data = &_sta_mgnt.info;
        user_buff.len = sizeof(struct rt_wlan_info);
        TIME_START();
        break;
    }
    case RT_WLAN_DEV_EVT_AP_START:
    {
        _ap_mgnt.state |= RT_WLAN_STATE_ACTIVE;
        user_event = RT_WLAN_EVT_AP_START;
        user_buff.data = &_ap_mgnt.info;
        user_buff.len = sizeof(struct rt_wlan_info);
        break;
    }
    case RT_WLAN_DEV_EVT_AP_STOP:
    {
        _ap_mgnt.state &= ~RT_WLAN_STATE_ACTIVE;
        user_event = RT_WLAN_EVT_AP_STOP;
        err = rt_wlan_sta_info_del_all(RT_WAITING_FOREVER);
        ......
        user_buff.data = &_ap_mgnt.info;
        user_buff.len = sizeof(struct rt_wlan_info);
        break;
    }
    case RT_WLAN_DEV_EVT_AP_ASSOCIATED:
    {
        user_event = RT_WLAN_EVT_AP_ASSOCIATED;
        if (user_buff.len != sizeof(struct rt_wlan_info))
            break;
        err = rt_wlan_sta_info_add(user_buff.data, RT_WAITING_FOREVER);
        ......
        break;
    }
    case RT_WLAN_DEV_EVT_AP_DISASSOCIATED:
    {
        user_event = RT_WLAN_EVT_AP_DISASSOCIATED;
        if (user_buff.len != sizeof(struct rt_wlan_info))
            break;
        err = rt_wlan_sta_info_del(user_buff.data, RT_WAITING_FOREVER);
        ......
        break;
    }
    case RT_WLAN_DEV_EVT_AP_ASSOCIATE_FAILED:
        break;
    case RT_WLAN_DEV_EVT_SCAN_REPORT:
    {
        user_event = RT_WLAN_EVT_SCAN_REPORT;
        if (user_buff.len != sizeof(struct rt_wlan_info))
            break;
        rt_wlan_scan_result_cache(user_buff.data, 0);
        break;
    }
    case RT_WLAN_DEV_EVT_SCAN_DONE:
    {
        user_buff.data = &scan_result;
        user_buff.len = sizeof(scan_result);
        user_event = RT_WLAN_EVT_SCAN_DONE;
        break;
    }
    default :
        return;
    }

    /* send event */
    COMPLETE_LOCK();
    for (i = 0; i < sizeof(complete_tab) / sizeof(complete_tab[0]); i++)
    {
        if ((complete_tab[i] != RT_NULL))
        {
            complete_tab[i]->event_flag |= 0x1 << event;
            rt_event_send(&complete_tab[i]->complete, 0x1 << event);
        }
    }
    COMPLETE_UNLOCK();
    /* Get user callback */
    if (user_event < RT_WLAN_EVT_MAX)
    {
        level = rt_hw_interrupt_disable();
        handler = event_tab[user_event].handler;
        user_parameter = event_tab[user_event].parameter;
        rt_hw_interrupt_enable(level);
    }

    /* run user callback fun */
    if (handler)
        handler(user_event, &user_buff, user_parameter);
}
```

当WLAN设备层有事件发生时，调用WLAN管理层注册的事件调度函数rt_wlan_event_dispatch，该函数根据事件类型对WLAN管理层的全局变量进行相应的配置，最后运行用户注册到WLAN管理层的事件回调函数（支持传入参数）。

在函数rt_wlan_event_dispatch中除了根据发生的事件类型调用相应的事件回调函数外，还向complete_tab[i]发送了事件，这是何意呢？再回头看WLAN管理层接口函数中名字和功能相近的几对儿：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_mgnt.h

/* 阻塞式连接热点，连接成功或失败后才会返回 */
rt_err_t rt_wlan_connect(const char *ssid, const char *password);
/* 非阻塞连接热点（advance），返回值仅表示连接动作是否开始执行，是否连接成功需要主动查询或设置回调通知 */
rt_err_t rt_wlan_connect_adv(struct rt_wlan_info *info, const char *password);

/* 阻塞式启动热点，返回值表示是否启动成功 */
rt_err_t rt_wlan_start_ap(const char *ssid, const char *password);
/* 非阻塞启动热点，热点是否启动需要手动查询或回调通知 */
rt_err_t rt_wlan_start_ap_adv(struct rt_wlan_info *info, const char *password);

/* 异步扫描函数，扫描完成需要通过回调进行通知 */
rt_err_t rt_wlan_scan(void);
/* 同步扫描函数，扫描全部热点信息，完成过直接返回扫描结果 */
struct rt_wlan_scan_result *rt_wlan_scan_sync(void);
```

WLAN管理层提供的连接热点、启动热点、扫描热点等都有阻塞/非阻塞、异步/同步两个版本，WLAN设备层或驱动库文件提供的接口函数都不负责同步或阻塞等待，而是直接异步/非阻塞执行。WLAN管理层提供的同步或阻塞等待接口需要该层自己实现，WLAN管理层也正是通过一个全局事件对象complete_tab实现同步/阻塞等待功能的。当调用同步或阻塞等待接口函数时，会先创建一个等待事件complete，接着执行对应的异步/非阻塞函数，阻塞等待接收事件complete；当设备层发生期望的事件后，调用执行函数rt_wlan_event_dispatch，在该函数中发送事件complete，然后释放事件complete资源并返回，实现同步或阻塞等待的目的。

## 六、WIFI与Socket网络开发示例
### 6.1 WLAN框架使用示例
在编写WIFI 连接示例程序前，先把之前介绍DFS文件系统和Sensor传感器管理框架时使用的例程整理下，只保留需要系统自动初始化的部分驱动代码，比如QSPI Flash驱动初始化、AHT10 Sensor初始化等。

**QSPI Flash驱动初始化代码**

```c
// projects\stm32l475_wifi_sample\applications\drv_qspi_flash.c

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
        LOG_I("flash already enter qspi mode.");
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
......
#endif /* BSP_USING_QSPI_FLASH */
```

 - **AHT10 Sensor 初始化代码**

```c
// projects\stm32l475_wifi_sample\applications\sensor_port.c

#include <board.h>

#ifdef PKG_USING_AHT10
#include "sensor_asair_aht10.h"

#define AHT10_I2C_BUS  "i2c1"

int rt_hw_aht10_port(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.dev_name  = AHT10_I2C_BUS;
    cfg.intf.user_data = (void *)AHT10_I2C_ADDR;

    rt_hw_aht10_init("aht10", &cfg);

    return RT_EOK;
}
INIT_ENV_EXPORT(rt_hw_aht10_port);
#endif
```

只保留上面这些代码，删除之前示例遗留的源文件，我们就可以开始编写WLAN 示例工程代码了。

 - **WLAN事件回调函数**

在编写WLAN工程代码前，我们再回顾下WLAN协议层的函数rt_wlan_prot_ready，该函数被LwIP 适配层的函数netif_is_ready调用，实际执行的是WLAN管理层的函数rt_wlan_prot_ready_event，也即当LwIP协议网络接口完成初始化后会调用执行函数rt_wlan_prot_ready_event，该函数实现代码如下：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_mgnt.c

int rt_wlan_prot_ready_event(struct rt_wlan_device *wlan, struct rt_wlan_buff *buff)
{
    rt_base_t level;
    void *user_parameter;
    rt_wlan_event_handler handler = RT_NULL;

    if ((wlan == RT_NULL) || (_sta_mgnt.device != wlan) || (!(_sta_mgnt.state & RT_WLAN_STATE_CONNECT)))
        return -1;
    if (_sta_mgnt.state & RT_WLAN_STATE_READY)
        return 0;
        
    level = rt_hw_interrupt_disable();
    _sta_mgnt.state |= RT_WLAN_STATE_READY;
    handler = event_tab[RT_WLAN_EVT_READY].handler;
    user_parameter = event_tab[RT_WLAN_EVT_READY].parameter;
    rt_hw_interrupt_enable(level);
    
    if (handler)
        handler(RT_WLAN_EVT_READY, buff, user_parameter);
    return 0;
}
```

函数rt_wlan_prot_ready_event 内部调用的是事件RT_WLAN_EVT_READY的回调处理函数，该函数需要我们在工程代码中自行实现并注册。当WLAN连接不稳定或发生故障断开连接时，也应能及时给出反馈信息提醒我们WIFI 已断开，因此也为事件RT_WLAN_EVT_STA_DISCONNECTED实现并注册一个回调函数。WLAN管理层支持的事件种类如下：

```c
// rt-thread-4.0.1\components\drivers\wlan\wlan_mgnt.h

typedef enum
{
    RT_WLAN_EVT_READY = 0,              /* connect and prot is ok, You can send data*/
    RT_WLAN_EVT_SCAN_DONE,              /* Scan a info */
    RT_WLAN_EVT_SCAN_REPORT,            /* Scan end */
    RT_WLAN_EVT_STA_CONNECTED,          /* connect success */
    RT_WLAN_EVT_STA_CONNECTED_FAIL,     /* connection failed */
    RT_WLAN_EVT_STA_DISCONNECTED,       /* disconnect */
    RT_WLAN_EVT_AP_START,               /* AP start */
    RT_WLAN_EVT_AP_STOP,                /* AP stop */
    RT_WLAN_EVT_AP_ASSOCIATED,          /* sta associated */
    RT_WLAN_EVT_AP_DISASSOCIATED,       /* sta disassociated */
    RT_WLAN_EVT_MAX
} rt_wlan_event_t;
```

我们在下面的示例工程代码中实现其中的两个事件RT_WLAN_EVT_READY和RT_WLAN_EVT_STA_DISCONNECTED的回调函数，其中事件RT_WLAN_EVT_READY回调函数涉及到LwIP 网络接口初始化就绪通知，我们使用信号量来实现LwIP初始化与网络访问间的同步顺序。当LwIP网络接口完成初始化调用执行RT_WLAN_EVT_READY事件回调函数，则释放信号量，主程序中等待LwIP网络就绪节点处获得该信号量，就可以继续执行，使用LwIP 协议栈提供的网络服务了。

 - **WLAN框架应用示例**

下面的示例工程想使用WLAN框架管理层、协议层、参数配置层这三部分提供的功能：首先是WLAN管理层提供的WIFI 扫描连接周围热点的功能；然后是WLAN协议层提供的网络访问功能，这里执行ifconfig命令和ping命令；最后是WLAN参数配置层提供的自动重连服务。按照这个业务逻辑，编写示例工程代码如下：

```c
// projects\stm32l475_wifi_sample\applications\main.c

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <msh.h>
#include "drv_wlan.h"
#include "wifi_config.h"
#include <stdio.h>
#include <stdlib.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define WLAN_SSID "_360WiFi_"
#define WLAN_PASSWORD "********"
#define NET_READY_TIME_OUT (rt_tick_from_millisecond(15 * 1000))

static void print_scan_result(struct rt_wlan_scan_result *scan_result);
static void print_wlan_information(struct rt_wlan_info *info);

static struct rt_semaphore net_ready;

/* WLAN网卡就绪回调函数 */
void wlan_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    rt_sem_release(&net_ready);
}

/* 断开连接回调函数 */
void wlan_station_disconnect_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    LOG_I("disconnect from the network!");
}

int main(void)
{
    int result = RT_EOK;
    struct rt_wlan_info info;
    struct rt_wlan_scan_result *scan_result;

    /* 等待 1000 ms 以便 wifi 完成初始化 */
    rt_hw_wlan_wait_init_done(1000);

    /* 扫描热点 */
    LOG_D("start to scan ap ...");
    /* 执行同步扫描 */
    scan_result = rt_wlan_scan_sync();
    if (scan_result)
    {
        LOG_D("the scan is complete, results is as follows: ");
        /* 打印扫描结果 */
        print_scan_result(scan_result);
        /* 清除扫描结果 */
        rt_wlan_scan_result_clean();
    }
    else
    {
        LOG_E("not found ap information ");
        return -1;
    }

    /* 热点连接 */
    LOG_D("start to connect ap ...");
    rt_sem_init(&net_ready, "net_ready", 0, RT_IPC_FLAG_FIFO);

    /* 注册 wlan ready 回调函数 */
    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wlan_ready_handler, RT_NULL);
    /* 注册 wlan 断开回调函数 */
    rt_wlan_register_event_handler(RT_WLAN_EVT_STA_DISCONNECTED, wlan_station_disconnect_handler, RT_NULL);

    /* 阻塞式连接指定热点 */
    result = rt_wlan_connect(WLAN_SSID, WLAN_PASSWORD);
    if (result == RT_EOK)
    {
        rt_memset(&info, 0, sizeof(struct rt_wlan_info));
        /* 获取当前连接热点信息 */
        rt_wlan_get_info(&info);
        LOG_D("station information:");
        print_wlan_information(&info);

        /* 等待成功获取 IP */
        result = rt_sem_take(&net_ready, NET_READY_TIME_OUT);
        if (result == RT_EOK)
        {
            LOG_D("networking ready!");
            msh_exec("ifconfig", rt_strlen("ifconfig"));
            rt_thread_mdelay(2000);
            msh_exec("ping www.baidu.com", rt_strlen("ping www.baidu.com"));
        }
        else
        {
            LOG_D("wait ip got timeout!");
        }
        /* 回收资源 */
        rt_wlan_unregister_event_handler(RT_WLAN_EVT_READY);
        rt_sem_detach(&net_ready);
    }
    else
    {
        LOG_E("The AP(%s) is connect failed!", WLAN_SSID);
    }

    rt_thread_mdelay(5000);

    LOG_D("ready to disconect from ap ...");
    rt_wlan_disconnect();

    /* 自动连接 */
    LOG_D("start to autoconnect ...");
    /* 初始化自动连接配置 */
    wlan_autoconnect_init();
    /* 使能 wlan 自动连接 */
    rt_wlan_config_autoreconnect(RT_TRUE);

    return 0;
}
```

为了方便在串口控制台交互WIFI 热点信息，这里实现了两个打印WIFI 热点信息的函数：print_scan_result 可以打印扫描到的所有WIFI 热点信息（可以参考函数rt_wlan_cfg_dump 的实现代码）；print_wlan_information可以打印指定的 WIFI 热点信息。这两个WIFI 热点信息打印函数的实现代码如下：

```c
static void print_scan_result(struct rt_wlan_scan_result *scan_result)
{
    char *security;
    int index, num;

    num = scan_result->num;
    /* 有规则的排列扫描到的热点 */
    rt_kprintf("             SSID                      MAC            security    rssi chn Mbps\n");
    rt_kprintf("------------------------------- -----------------  -------------- ---- --- ----\n");
    for (index = 0; index < num; index++)
    {
        rt_kprintf("%-32.32s", &scan_result->info[index].ssid.val[0]);
        rt_kprintf("%02x:%02x:%02x:%02x:%02x:%02x  ",
                   scan_result->info[index].bssid[0],
                   scan_result->info[index].bssid[1],
                   scan_result->info[index].bssid[2],
                   scan_result->info[index].bssid[3],
                   scan_result->info[index].bssid[4],
                   scan_result->info[index].bssid[5]);
        switch (scan_result->info[index].security)
        {
        case SECURITY_OPEN:
            security = "OPEN";
            break;
        case SECURITY_WEP_PSK:
            security = "WEP_PSK";
            break;
        case SECURITY_WEP_SHARED:
            security = "WEP_SHARED";
            break;
        case SECURITY_WPA_TKIP_PSK:
            security = "WPA_TKIP_PSK";
            break;
        case SECURITY_WPA_AES_PSK:
            security = "WPA_AES_PSK";
            break;
        case SECURITY_WPA2_AES_PSK:
            security = "WPA2_AES_PSK";
            break;
        case SECURITY_WPA2_TKIP_PSK:
            security = "WPA2_TKIP_PSK";
            break;
        case SECURITY_WPA2_MIXED_PSK:
            security = "WPA2_MIXED_PSK";
            break;
        case SECURITY_WPS_OPEN:
            security = "WPS_OPEN";
            break;
        case SECURITY_WPS_SECURE:
            security = "WPS_SECURE";
            break;
        default:
            security = "UNKNOWN";
            break;
        }
        rt_kprintf("%-14.14s ", security);
        rt_kprintf("%-4d ", scan_result->info[index].rssi);
        rt_kprintf("%3d ", scan_result->info[index].channel);
        rt_kprintf("%4d\n", scan_result->info[index].datarate / 1000000);
    }
    rt_kprintf("\n");
}

static void print_wlan_information(struct rt_wlan_info *info)
{
    LOG_D("SSID : %-.32s", &info->ssid.val[0]);
    LOG_D("MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x", info->bssid[0],
          info->bssid[1],
          info->bssid[2],
          info->bssid[3],
          info->bssid[4],
          info->bssid[5]);
    LOG_D("Channel: %d", info->channel);
    LOG_D("DataRate: %dMbps", info->datarate / 1000000);
    LOG_D("RSSI: %d", info->rssi);
}
```

到这里我们想要实现的示例工程代码就编写完了，在生成工程之前，先通过menuconfig命令启用AP6181 WIFI 设备，配置界面如下：
![启用AP6181 WIFI 网卡](https://img-blog.csdnimg.cn/20200410192310219.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
启用了AP6181 WIFI 设备，由于依赖项RT_USING_LWIP，也同时启用了LwIP协议栈。我们在示例工程中使用了两个命令ifconfig 和 ping 是由netdev组件提供的，也需要启用netdev组件；为方便后面使用BSD Socket 进行网络编程，还需要启用SAL组件，启用netdev组件与SAL组件的配置界面如下：
![启用netdev组件](https://img-blog.csdnimg.cn/20200410193814325.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
在env环境中执行scons --target=mdk5 命令，生成MDK5工程文件，打开project.uvprojx，编译无报错，将程序烧录到Pandora开发板中，输出信息如下：
![AP6181 WIFI 示例程序执行结果](https://img-blog.csdnimg.cn/20200410194621671.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
从上面程序执行的输出结果可以看出，AP6181 WIFI 模块工作正常，我们对WLAN管理框架各部分的移植适配也没什么问题。

 - **WLAN框架提供的msh命令使用示例**

WLAN管理框架也为我们提供了msh命令，方便我们进行调试，msh命令使用示例如下：
![WIFI MSH命令使用示例](https://img-blog.csdnimg.cn/20200410195804642.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)

### 6.2 BSD Socket 网络编程示例
我们使用博客：[网络分层结构 + netdev/SAL原理](https://blog.csdn.net/m0_37621078/article/details/104836942)中的HTTP服务应用示例程序代码，将从Pandora开发板 AHT10 Sensor读取的温湿度数据通过HTTP服务返回给远程访问的Web客户端，再次贴出HTTP服务示例代码如下：

```c
// projects\stm32l475_wifi_sample\applications\sockapi_http_demo.c

#include <rtthread.h>
#include <sys\socket.h>		/* 使用BSD socket，需要包含socket.h头文件 */
#include <string.h>
#include <sensor.h>

#define DBG_TAG               "Socket"
#define DBG_LVL               DBG_INFO
#include <rtdbg.h>

/* defined received buffer size */
#define BUFSZ       512
/* defined the number of times aht10 sensor data is sent */
#define SENDCNT     10
/* defined aht10 sensor name */
#define SENSOR_TEMP_NAME    "temp_aht10"
#define SENSOR_HUMI_NAME    "humi_aht10"

static rt_thread_t tid = RT_NULL;

const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char http_index_html[] = "<html><head><title>Congrats!</title></head>\
                                <body><h1>Welcome to LwIP 2.1.0 HTTP server!</h1></body></html>";
static char Sensor_Data[] ="<html><head><title>Congrats!</title></head>\
                                <center><p>The current temperature is: %3d.%d C, humidity is: %3d.%d %.\
                                </center></body></html>";

/** Serve one HTTP connection accepted in the http thread */
static void httpserver_serve(int sock)
{
    /* 用于接收的指针，后面会做一次动态分配以请求可用内存 */
    char *buffer;
    int bytes_received, cnt = SENDCNT;
    /* sensor设备对象与sensor数据类型 */
    rt_device_t sensor_temp, sensor_humi;
    struct rt_sensor_data temp_data, humi_data;

    /* 分配接收用的数据缓冲 */
    buffer = rt_malloc(BUFSZ+1);
    if(buffer == RT_NULL)
    {
      LOG_E("No memory\n");
      return;
    }

    /* 从connected socket中接收数据，接收buffer是512大小 */
    bytes_received = recv(sock, buffer, BUFSZ, 0);
    if (bytes_received > 0)
    {
        /* 有接收到数据，在末端添加字符串结束符 */
        buffer[bytes_received] = '\0';

        /* 若是GET请求，则向网页返回html数据 */
        if(strncmp(buffer, "GET", 3) == 0)
        {
            /* 向网页返回固定数据 */
            send(sock, http_html_hdr, strlen(http_html_hdr), 0);
            send(sock, http_index_html, strlen(http_index_html), 0);

            /* 发现并打开温湿度传感器设备 */
            sensor_temp = rt_device_find(SENSOR_TEMP_NAME);
            rt_device_open(sensor_temp, RT_DEVICE_FLAG_RDONLY);

            sensor_humi = rt_device_find(SENSOR_HUMI_NAME);
            rt_device_open(sensor_humi, RT_DEVICE_FLAG_RDONLY);

            do
            {
                rt_thread_mdelay(5000);
                /* 读取温湿度数据，并将其填入Sensor_Data字符串 */
                rt_device_read(sensor_temp, 0, &temp_data, 1);
                rt_device_read(sensor_humi, 0, &humi_data, 1);
                rt_sprintf(buffer, Sensor_Data, 
                        temp_data.data.temp / 10, temp_data.data.temp % 10,
                        humi_data.data.humi / 10, humi_data.data.humi % 10);
                
                /* 向网页周期性发送温湿度数据 */
                send(sock, buffer, strlen(buffer), 0);
            } while (cnt--);

            rt_device_close(sensor_temp);
            rt_device_close(sensor_humi);
        }
    }
    rt_free(buffer);
}

/** The main function, never returns! */
static void httpserver_thread(void *arg)
{
    socklen_t sin_size;
    int sock, connected, ret;
    struct sockaddr_in server_addr, client_addr;

    /* 一个socket在使用前，需要预先创建出来，指定SOCK_STREAM为TCP的socket */
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if(sock == -1)
    {
      LOG_E("Socket error\n"); 
      return;
    }

    /* 初始化服务端地址，HTTP端口号为80 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* 绑定socket到服务端地址 */
    ret = bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
    if(ret == -1)
    {
      LOG_E("Unable to bind\n");   
      return;
    }

    /* 在socket上进行监听 */
    if(listen(sock, 5) == -1)
    {
      LOG_E("Listen error\n"); 
      return;
    }

    LOG_I("\nTCPServer Waiting for client on port 80...\n");
    
    do {
        sin_size = sizeof(struct sockaddr_in);
        /* 接受一个客户端连接socket的请求，这个函数调用是阻塞式的 */
        connected = accept(sock, (struct sockaddr *)&client_addr, &sin_size);
        if (connected >= 0)
        {
            /* 接受返回的client_addr指向了客户端的地址信息 */
            LOG_I("I got a connection from (%s , %d)\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            /* 客户端连接的处理 */
            httpserver_serve(connected);
            closesocket(connected);
        }
    } while(connected >= 0);
    
    closesocket(sock);
    return;
}

/** Initialize the HTTP server (start its thread) */
static void httpserver_init()
{
    /* 创建并启动http server线程 */
	tid = rt_thread_create("http_server_socket", 
  							httpserver_thread, NULL, 
							RT_LWIP_TCPTHREAD_STACKSIZE * 2, 
                            RT_LWIP_TCPTHREAD_PRIORITY + 1, 10);
	if(tid != RT_NULL)
	{
		rt_thread_startup(tid);
		LOG_I("Startup a tcp web server.\n");
	}
}
MSH_CMD_EXPORT_ALIAS(httpserver_init, sockapi_web, socket api httpserver init);
```

在env环境中使用scons --target=mdk5命令重新生产MDK5工程，编译无报错，将程序烧录到Pandora开发板，示例程序执行结果如下：
![HTTP Server示例程序执行结果](https://img-blog.csdnimg.cn/20200410201329214.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)




# 更多文章：

 - 《[IOT-OS之RT-Thread（十七）--- 如何使用HTTP协议实现OTA空中升级](https://blog.csdn.net/m0_37621078/article/details/105442358)》
 - 《[IOT-OS之RT-Thread（十五）--- SDIO设备对象管理 + AP6181(BCM43362) WiFi模块](https://blog.csdn.net/m0_37621078/article/details/105097567)》
 - 《[STM32之CubeL4（四）--- SD/MMC + SDIO + HAL](https://blog.csdn.net/m0_37621078/article/details/105093404)》
 - 《[IOT-OS之RT-Thread（十四）--- AT命令集 + ESP8266 WiFi模块](https://blog.csdn.net/m0_37621078/article/details/104973297)》
 - [《IOT-OS之RT-Thread（十三）--- 网络分层结构 + netdev/SAL原理》](https://blog.csdn.net/m0_37621078/article/details/104836942)
 - 《[IOT-OS之RT-Thread（十二）--- 驱动分层与主从分离思想](https://blog.csdn.net/m0_37621078/article/details/104790217)》
 - [《IOT-OS之RT-Thread（十一）--- FAL分区管理与easyflash变量管理》](https://blog.csdn.net/m0_37621078/article/details/102689903)
 - 《[Device File System文件系统管理](https://github.com/StreamAI/RT-Thread_Projects/tree/master/projects/stm32l475_dfs_sample)》
