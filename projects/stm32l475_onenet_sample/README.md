# 如何使用MQTT 协议实现OneNET 远程监控？

@[TOC]

> 物联网时代，我们想把周边的嵌入式设备都接入网络，依托云平台提供的各种服务，实现对嵌入式设备的远程监测和控制。前篇博文介绍了[如何实现Bootloader OTA 升级？](https://github.com/StreamAI/RT-Thread_Projects/tree/master/projects/stm32l475_ota_sample) 这也是物联网设备应该实现的一个重要功能，除了远程固件或应用升级功能外，物联网设备更常用的功能是将采集到的数据上传到云服务平台，并从云服务平台接收并执行控制指令，该如何将嵌入式设备接入云服务平台并实现远程监控功能呢？

# 一、设备怎么接入OneNET 物联网平台?
随着物联网、云计算、大数据、人工智能等概念的流行，为物联网设备提供云服务的平台也越来越丰富，比如国外的有AWS IoT、Azure IoT、IBM Watson IoT、Google Cloud IoT 等，国内的有中国移动OneNET、阿里云IoT、百度天工IoT、小米IoT、华为云IoT 等。

物联网云服务从结构上大概可以分为“云-管-端” 三个层级，为了更高效的支撑大规模物联网设备的接入，通常还加入了“边缘计算” 层，因此也常将其分为“云-网-边-端” 四个层级。

本文选择较为简单易用的中国移动OneNET 作为设备接入的IoT 云服务平台，该平台的“云-网-边-端”整体架构如下：
![OneNET 云-网-边-端 整体架构](https://img-blog.csdnimg.cn/20210513220930473.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
本文示例在 “云-管-端” 各层的选择：

 - **端-设备**：接入到物联网络中的各种嵌入式设备或智能终端，要接入云平台通常需要在设备端工程代码中集成目标云平台的SDK 代码。本文选用Pandora IoT 开发板，芯片型号为STM32L475；
 - **管-网络**：终端设备与云平台之间的数据通道，主要是承载在通信介质上的下层通信协议，比如Ethernet、Wi-Fi、BLE、NB-IoT、Cellular 等。本文选择Wi-Fi 作为终端设备接入Internet 的通信协议；
 - **云平台**：提供高效计算、消息传递、数据存储分析、智能识别等服务的云平台，实际是统一管理调度的服务器集群。本文选择接入OneNET 的应用层协议是MQTT，需要使用OneNET 的设备管理、数据流展示、下发命令等功能。

我们开发的嵌入式设备要接入OneNET 云平台，大概需要怎样的接入流程呢？我们可以从[中移动OneNET 开发文档](https://open.iot.10086.cn/doc/book/device-develop/multpro/MQTT/MQTT-manual.html)中找到答案。

我们选择MQTT 作为接入OneNET 的应用层消息传递协议，OneNET 提供了[新版MQTTS](https://open.iot.10086.cn/doc/mqtt/book/device-develop/manual.html) （在MQTT 物联网套件选项中，支持TLS 加密和认证，port 为8883）接入和[旧版MQTT](https://open.iot.10086.cn/doc/multiprotocol/book/develop/mqtt/device/manual.html)（在多协议接入选项中，不使用TLS 加密，port 为1883） 接入两种方式。

在前篇博文介绍[如何实现Bootloader OTA 升级](https://blog.csdn.net/m0_37621078/article/details/105442358)时谈到，我们的Pandora 开发板在添加mbedTLS 组件后编译烧录工程提示空间不足（除去Bootloader 的64KB 空间，留给Application 的只剩448KB，MbedTLS 要占用超过100KB），因此本文选择使用旧版MQTT 接入OneNET 云平台。使用旧版MQTT 协议接入OneNET 的开发流程如下：
![OneNET MQTT 接入流程](https://img-blog.csdnimg.cn/20210514003142285.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
我们开发的嵌入式设备接入OneNET 云平台，主要分为平台域配置和设备域集成SDK 两部分，OneNET 为开发者接入云平台提供了[设备模拟器](https://open.iot.10086.cn/doc/multiprotocol/book/develop/mqtt/device/doc-tool.html)，方便我们调试分析。我们先在平台域创建产品和设备，然后使用设备模拟器连接平台域创建的设备，并尝试上传数据点和下发命令，待通讯正常后再在我们的开发板上集成相应 SDK 并接入OneNET。

## 1.1 OneNET 平台域配置
OneNET 平台域的[账户注册](https://open.iot.10086.cn/doc/multiprotocol/book/get-start/login.html)可以参阅开发者文档，旧版MQTT 选择“前往旧版控制台” --> 选择"多协议接入" --> 选择“MQTT（旧版）” --> “添加产品”（可参阅文档[创建产品](https://open.iot.10086.cn/doc/multiprotocol/book/get-start/product&device/product-create.html)），添加后的产品如下：
![OneNET 添加产品](https://img-blog.csdnimg.cn/20210514164226682.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
我们在OneNET 上添加的产品，需要记住“产品ID” 和“Master-APIKey” 等信息（access_key 用于MQTTS 更安全的鉴权方式，本文示例用不着），这是我们将设备接入OneNET 的身份认证信息之一。

接下来在OneNET 上创建设备，选择“设备列表” --> “添加设备” （可参阅文档[创建设备](https://open.iot.10086.cn/doc/multiprotocol/book/get-start/product&device/device-create/single-device.html)），“鉴权信息” 推荐填写设备的唯一生产序列号，我们这里填写当前的时间戳，添加后的设备如下（手动“添加APIKey”，弹窗“APIKey” 输入的是“Device1_APIKey1”）：
![OneNET 创建设备](https://img-blog.csdnimg.cn/20210514165925993.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
我们在OneNET 上添加的产品，需要记住“设备ID” 、“鉴权信息”、“APIKey” 等信息，这也是我们将设备接入OneNET 的身份认证信息之一。

到这里就完成了OneNET 平台域产品和设备的创建，接下来我们使用OneNET 提供的设备模拟器“simulate-device.exe” 尝试接入OneNET，并尝试上传数据点、响应下发命令。

## 1.2 OneNET 设备模拟器上传数据点与下发命令
从上面OneNET 提供的开发流程图可知，旧版MQTT 接入的IP 地址是“183.230.40.39”，端口号是“6002”（不清楚为何没采用MQTT 协议标准的1883 端口号）。设备要接入OneNET，还需要我们创建的“设备ID”、“产品ID”、“鉴权信息”（也即设备编号） 等参数，我们将以上信息配置到设备模拟器中，点击”Connect“，成功连接到我们在平台域创建的设备：
![OneNET 设备模拟器接入云平台](https://img-blog.csdnimg.cn/20210514193503203.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
接下来如何向OneNET 上传数据点、如何响应OneNET 下发的指令呢？点击”[OneNET]上传数据点“ 界面，发现有7种数据类型，我们该选择哪种数据类型、编辑怎样的Json 数据才能正确上传呢？

我们可以查看OneNET 提供的[”设备终端接入协议-MQTT“](https://open.iot.10086.cn/doc/multiprotocol/book/develop/mqtt/device/doc-tool.html)文档，在“5.2.1 数据点上报” 一节介绍了支持上报的7 种Json 数据类型格式。我们尝试上传温湿度浮点型数据，每次上传一个数据就行了，数据平台默认以时序存储接收到的数据，因此我们不需要使用分隔符或时间戳，剩下 3 种Json 数据类型。Json 格式1 每个数据流可以同时上传多个数据点，格式略复杂，Json 格式3 需要带日期时间，Json 格式2 只需要datastream_id 和value 两个字段，比较简单且满足我们的需求，因此我们选择Json 格式2（也即数据类型3），按照示例格式上传温湿度数据如下：
![向OneNET 上传数据点](https://img-blog.csdnimg.cn/20210514194842271.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
从OneNET 下发命令不像上传数据点这么麻烦，向OneNET 上传数据点需要按照OneNET 定义的数据类型才能被云平台解析，从OneNET 下发命令则根据自己需求定义数据结构就行了，实际上就是将命令作为字符串原样传输给设备端了，设备端接收到命令字符串根据预设行为做出响应，从OneNET 下发命令的图示如下：
![从OneNET 下发命令到设备端](https://img-blog.csdnimg.cn/20210514200009681.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
我们使用OneNET 设备模拟器成功接入云平台，上传数据点和下发命令都正常，接下来就需要基于我们的开发板集成OneNET SDK，尝试接入OneNET 云平台，并上传温湿度数据，响应从OneNET 下发的指令了。

# 二、Paho-MQTT 的实现原理是什么？
我们在前面[http_ota 示例工程](https://github.com/StreamAI/RT-Thread_Projects/tree/master/projects/stm32l475_ota_sample) 的基础上继续开发，OneNET 提供了[接入其云平台的SDK](https://github.com/cm-heclouds/MQTT/tree/master/mqtt_sdk)，我们可以将其集成到我们的工程代码中，中间少不了一些移植操作。既然我们使用的RT-Thread 系统提供了丰富的第三方组件，我们可以先在[http://packages.rt-thread.org/](http://packages.rt-thread.org/)搜索下关键字“OneNET”，确实搜到了[OneNET 组件包](https://github.com/RT-Thread-packages/onenet/blob/master/README_ZH.md)。既然RT-Thread 提供了OneNET 组件包，移植工作量就小多了，本文直接使用RT-Thread 提供的组件包。

从[OneNET 组件包](https://github.com/RT-Thread-packages/onenet/blob/master/README_ZH.md)的介绍看，它依赖[paho-mqtt](https://github.com/RT-Thread-packages/paho-mqtt) 和[cJSON 组件包](https://github.com/RT-Thread-packages/cJSON)，paho-mqtt 是接入OneNET 的MQTT Client，cJSON 是一个生成或解析JSON 格式数据的C 语言库（OneNET 上传数据点需要Json 数据类型）。

Paho-mqtt 是在[Eclipse Paho project](https://projects.eclipse.org/projects/iot.paho) 项目[MQTT Client embedded-c 语言实现版本](https://github.com/eclipse/paho.mqtt.embedded-c)的基础上移植来的，Eclipse Paho project 提供了主流编程语言的MQTT Client 实现版本，在资源比较受限的嵌入式设备中（比如STM32L475）常选用paho.mqtt.embedded-c 版本，该版本支持的功能特性如下：
![Eclipse Paho MQTT Client Comparison](https://img-blog.csdnimg.cn/20210515002743303.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
由于paho.mqtt.embedded-c 常用于资源受限的嵌入式设备，很多功能并没有实现，也不支持MQTT 5.0 新特性。本文使用MQTT 3.1.1 版本协议，虽然paho-mqtt 支持TLS 加密，受限于我们设备的存储空间，就不使用TLS 加密了。我们通过menuconfig 将paho-mqtt 组件包添加进stm32l475_onenet_sample 工程中（从stm32l475_ota_sample 工程复制而来），配置界面如下：
![添加Paho MQTT 组件包](https://img-blog.csdnimg.cn/20210515005313498.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
保存配置并退出menuconfig，自动从github 仓库下载paho-mqtt 软件包到我们的工程中，在env 环境中执行“scons --target=mdk5” 命令生成Keil MDK5 project。前篇博文已经详细介绍了[MQTT 协议的设计原理和报文格式](https://blog.csdn.net/m0_37621078/article/details/116246058)，这里简单介绍下paho-mqtt 的大概实现过程，以及移植工作。

## 2.1 Paho-MQTT 订阅-发布实现逻辑

 - **Paho-MQTT Client Session 数据结构**

Paho-mqtt 是工作在 TCP/IP 协议之上的，而且是基于连接的，因此需要先使用socket API 建立TCP 连接。既然MQTT 协议也需要建立并维持连接状态，paho-mqtt 也应该为client session 设计一个数据结构用来记录连接状态信息，MQTTClient 的数据结构定义如下（主要字段已添加注释）：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\packages\pahomqtt-v1.1.0\MQTTClient-RT\paho_mqtt.h

struct MQTTClient
{
	/** uri 存储建立TCP或TLS连接的 ip_address:port，比如 "tcp://test.mosquitto.org:1883" 或"ssl://test.mosquitto.org:8883"，
		sock 存储为TCP 或TLS 连接创建的socket 套接字 */
    const char *uri;		
    int sock;				
	/* MQTT CONNECT 报文中包含的字段信息，包括clientID、User name、Password、keepAliveInterval、Will data 等 */
    MQTTPacket_connectData condata;		

    unsigned int next_packetid, command_timeout_ms;
    size_t buf_size, readbuf_size;
    unsigned char *buf, *readbuf;
    unsigned int keepAliveInterval;
    int connect_timeout;
    int reconnect_interval;
    int isblocking;
    int isconnected;
    uint32_t tick_ping;
	/* 用于通知上层应用 MQTT连接状态变更的回调函数，比如MQTT Client连接、上线、下线时做出什么行为 */
    void (*connect_callback)(MQTTClient *);
    void (*online_callback)(MQTTClient *);
    void (*offline_callback)(MQTTClient *);
	/* MQTT Client 订阅主题时，同时为每个MQTT主题名或主题过滤器注册一个回调函数，
	  当该主题有消息到来时自动执行相应主题的回调函数处理接收到的消息 */
    struct MessageHandlers
    {
        char *topicFilter;
        void (*callback)(MQTTClient *, MessageData *);
        enum QoS qos;
    } messageHandlers[MAX_MESSAGE_HANDLERS]; /* Message handlers are indexed by subscription topic */
    /* 设置默认的消息处理函数，当上面的主题都不匹配时，执行这个默认消息处理函数 */
    void (*defaultMessageHandler)(MQTTClient *, MessageData *);		

    /* publish interface */
    rt_mutex_t pub_mutex;             /* publish data mutex for blocking */
#if defined(RT_USING_POSIX) && (defined(RT_USING_DFS_NET) || defined(SAL_USING_POSIX))
	/* 使用 pipe 管道设备将应用线程要发布的消息传递给paho_mqtt 线程，
	   pipe 内部是一个环形缓冲区，其中pub_pipe[0] 是读取消息的端口，pub_pipe[1] 是写入消息的端口 */
    struct rt_pipe_device* pipe_device;
    int pub_pipe[2];
#else
	/* 使用socket UDP 将应用线程要发布的消息传递给paho_mqtt 线程，应用线程将要发布的消息发送到本地网卡的pub_port 端口，
	   paho_mqtt 线程 则从本地网卡的pub_port 端口读取消息并发布 */
    int pub_sock;
    int pub_port;
#endif /* RT_USING_POSIX && (RT_USING_DFS_NET || SAL_USING_POSIX) */

#ifdef MQTT_USING_TLS
    MbedTLSSession *tls_session;      /* mbedtls session struct */
#endif
	
	void *user_data;                  /* user-specific data */
};

typedef struct
{
	/** The eyecatcher for this structure.  must be MQTC. */
	char struct_id[4];
	/** The version number of this structure.  Must be 0 */
	int struct_version;
	/** Version of MQTT to be used.  3 = 3.1 4 = 3.1.1  */
	unsigned char MQTTVersion;
	MQTTString clientID;
	unsigned short keepAliveInterval;
	unsigned char cleansession;
	unsigned char willFlag;
	MQTTPacket_willOptions will;
	MQTTString username;
	MQTTString password;
} MQTTPacket_connectData;

/* MQTT 消息都是基于主题的，因此将消息内容跟主题名放到一个结构体中，消息内容包含QoS、ID、retained、dup、payload 等字段 */
typedef struct MessageData
{
    MQTTMessage *message;
    MQTTString *topicName;
} MessageData;

typedef struct MQTTMessage
{
    enum QoS qos;
    unsigned char retained;
    unsigned char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
} MQTTMessage;
```

MQTT 协议的主要功能可以分为连接/保活连接/断开连接、订阅/退订主题、发布消息这三个部分：MQTTClient 数据结构中前半部分成员变量主要跟连接管理有关，比如uri、sock、condata、keepAliveInterval、tick_ping 等；中间部分主要维护了订阅主题列表及其对应的消息处理函数指针，比如messageHandlers[i]、defaultMessageHandler 等；后半部分主要用来管理应用线程与paho_mqtt 线程之间的消息传递，paho-mqtt 组件包提供了两种线程间消息传递方式，一种是通过pipe 管道设备，另一种是通过socket UDP 端口。

RT-Thread 为paho-mqtt 消息处理专门创建了一个线程paho_mqtt_thread，我们发布消息一般是在另外的应用线程中，线程间消息传递通常有管道、消息队列、共享内存、socket 等，paho-mqtt 提供了pipe 管道和socket udp 两种线程间消息传递方式，将要发布的消息从应用线程传递到paho_mqtt_thread 线程。如果你熟悉 lwip 协议栈，会知道 lwip 也为网络数据包的处理专门创建了一个内核线程tcpip_thread，[用户线程与tcpip_thread 线程之间的消息传递是通过邮箱和共享内存实现的](https://blog.csdn.net/m0_37621078/article/details/98465308)，跟消息队列的实现方式类似，不过减少了消息内容的复制，性能更高些。

 - **Paho-MQTT 订阅-发布实现逻辑**

Paho-mqtt 组件库是在用户配置完MQTT 连接参数后调用函数paho_mqtt_start 启动一个MQTT Client 的，该函数主要是创建了一个线程paho_mqtt_thread 来处理MQTT 连接会话的创建、预设主题的订阅、订阅主题消息的监听和处理、待发布消息的监听和发布、心跳保活报文的周期性发送等任务，该函数的实现代码如下（主要函数调用已添加注释，注释以TCP 连接而非TLS 连接为例）：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\packages\pahomqtt-v1.1.0\MQTTClient-RT\paho_mqtt_pipe.c

static void paho_mqtt_thread(void *param)
{
    MQTTClient *c = (MQTTClient *)param;
    int i, rc, len;
    int rc_t = 0;

    /* create publish pipe */
    c->pipe_device = mqtt_pipe_init(c->pub_pipe);
    if (c->pipe_device == RT_NULL)
        goto _mqtt_exit;

_mqtt_start:
    if (c->connect_callback)
        c->connect_callback(c);

	/* 解析c->uri，并调用socket API 建立TCP 或TLS 连接，比如通过调用connect 建立TCP 连接 */
    rc = net_connect(c);
    if (rc != 0)
    {
        LOG_E("Net connect error(%d).", rc);
        goto _mqtt_restart;
    }
	/* 通过函数MQTTSerialize_connect 构造MQTT CONNECT 报文，调用函数send 将CONNECT 报文发送出去，
	   调用select 函数监听MQTT CONNACK 报文，并通过函数recv 读取接收到的报文，
	   调用函数MQTTDeserialize_connack 解析CONNACK 报文，返回值是CONNACK 中的Reason Code */
    rc = MQTTConnect(c);
    if (rc != 0)
    {
        LOG_E("MQTT connect error(%d): %s.", rc, MQTTSerialize_connack_string(rc));
        goto _mqtt_restart;
    }

    LOG_I("MQTT server connect success.");
    
	/* 将创建MQTTClient 是预设的订阅主题通过函数MQTTSubscribe 构造SUBSCRIBE 报文发送出去 */
    for (i = 0; i < MAX_MESSAGE_HANDLERS; i++)
    {
        const char *topic = c->messageHandlers[i].topicFilter;
        enum QoS qos = c->messageHandlers[i].qos;

        if (topic == RT_NULL)
            continue;
		/* 通过函数MQTTSerialize_subscribe 构造MQTT SUBSCRIBE 报文，调用函数send 将SUBSCRIBE 报文发送出去，
	       调用select 函数监听MQTT SUBACK 报文，并通过函数recv 读取接收到的报文，
	       调用函数MQTTDeserialize_suback 解析SUBACK 报文，返回值是SUBACK 中的Reason Code */
        rc = MQTTSubscribe(c, topic, qos);
        LOG_I("Subscribe #%d %s %s!", i, topic, (rc < 0) || (rc == 0x80) ? ("fail") : ("OK"));

        if (rc != 0)
        {
            if (rc == 0x80)
                LOG_E("QoS(%d) config err!", qos);
            goto _mqtt_disconnect;
        }
    }

    if (c->online_callback)
        c->online_callback(c);

    c->tick_ping = rt_tick_get();
    while (1)
    {
        int res;
        rt_tick_t tick_now;
        fd_set readset;
        struct timeval timeout;

        tick_now = rt_tick_get();
        if (((tick_now - c->tick_ping) / RT_TICK_PER_SECOND) > (c->keepAliveInterval - 5))
            timeout.tv_sec = 1;
        else
            timeout.tv_sec = c->keepAliveInterval - 10 - (tick_now - c->tick_ping) / RT_TICK_PER_SECOND;
        timeout.tv_usec = 0;
        
		/* 调用函数select 监听c->sock 和c->pub_pipe[0]，其中c->sock 是本地Client 与远端Server 建立连接的socket，
		    c->pub_pipe[0] 是pipe 管道设备的读取端口，可以从该端口读取应用线程写入到c->pub_pipe[1] 的消息 */
        FD_ZERO(&readset);
        FD_SET(c->sock, &readset);
        FD_SET(c->pub_pipe[0], &readset);

        /* int select(maxfdp1, readset, writeset, exceptset, timeout); */
        res = select(((c->pub_pipe[0] > c->sock) ? c->pub_pipe[0] : c->sock) + 1,
                     &readset, RT_NULL, RT_NULL, &timeout);
        /* 当select 没有监听到数据到来时，通过函数MQTTSerialize_pingreq 构造MQTT PINGREQ 报文，
            调用函数send 将PINGREQ 报文发送出去，调用select 函数监听MQTT PINGRESP 报文 */
        if (res == 0)
        {
            len = MQTTSerialize_pingreq(c->buf, c->buf_size);
            rc = sendPacket(c, len);
            if (rc != 0)
            {
                LOG_E("[%d] send ping rc: %d ", rt_tick_get(), rc);
                goto _mqtt_disconnect;
            }

            /* wait Ping Response. */
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            FD_ZERO(&readset);
            FD_SET(c->sock, &readset);

            res = select(c->sock + 1, &readset, RT_NULL, RT_NULL, &timeout);
            if (res <= 0)
            {
                LOG_E("[%d] wait Ping Response res: %d", rt_tick_get(), res);
                goto _mqtt_disconnect;
            }
        } /* res == 0: timeount for ping. */

        if (res < 0)
        {
            LOG_E("select res: %d", res);
            goto _mqtt_disconnect;
        }
		/* 当select 监听到c->sock 有数据到来时，调用函数MQTT_cycle 处理从远端Server 接收到的MQTT 报文，该函数后面再详细解释 */
        if (FD_ISSET(c->sock, &readset))
        {
            //LOG_D("sock FD_ISSET");
            rc_t = MQTT_cycle(c);
            //LOG_D("sock FD_ISSET rc_t : %d", rc_t);
            if (rc_t < 0)    goto _mqtt_disconnect;

            continue;
        }
		/* 当select 监听到c->pub_pipe[0] 有数据到来时，先调用函数read 读取应用数据，若数据是“DISCONNECT” 指令则断开MQTT 连接，
		  若是需要发布的数据，通过函数MQTTSerialize_publish 构造MQTT PUBLISH 报文，调用函数send 将PUBLISH 报文发送出去 */
        if (FD_ISSET(c->pub_pipe[0], &readset))
        {
            MQTTMessage *message;
            MQTTString topic = MQTTString_initializer;

            //LOG_D("pub_sock FD_ISSET");

            len = read(c->pub_pipe[0], c->readbuf, c->readbuf_size);

            if (len < sizeof(MQTTMessage))
            {
                c->readbuf[len] = '\0';
                LOG_D("pub_sock recv %d byte: %s", len, c->readbuf);

                if (strcmp((const char *)c->readbuf, "DISCONNECT") == 0)
                    goto _mqtt_disconnect_exit;

                continue;
            }

            message = (MQTTMessage *)c->readbuf;
            message->payload = c->readbuf + sizeof(MQTTMessage);
            topic.cstring = (char *)c->readbuf + sizeof(MQTTMessage) + message->payloadlen;
            //LOG_D("pub_sock topic:%s, payloadlen:%d", topic.cstring, message->payloadlen);

            len = MQTTSerialize_publish(c->buf, c->buf_size, 0, message->qos, message->retained, message->id,
                                        topic, (unsigned char *)message->payload, message->payloadlen);
            if (len <= 0)
            {
                LOG_D("MQTTSerialize_publish len: %d", len);
                goto _mqtt_disconnect;
            }

            if ((rc = sendPacket(c, len)) != PAHO_SUCCESS) // send the subscribe packet
            {
                LOG_D("MQTTSerialize_publish sendPacket rc: %d", rc);
                goto _mqtt_disconnect;
            }

            if (c->isblocking && c->pub_mutex)
                rt_mutex_release(c->pub_mutex);
        } /* pbulish sock handler. */
    } /* while (1) */

_mqtt_disconnect:
    MQTTDisconnect(c);
_mqtt_restart:
    if (c->offline_callback)
        c->offline_callback(c);

    net_disconnect(c);
    rt_thread_delay(c->reconnect_interval > 0 ? 
        rt_tick_from_millisecond(c->reconnect_interval) : RT_TICK_PER_SECOND * 5);
    LOG_D("restart!");
    goto _mqtt_start;

_mqtt_disconnect_exit:
    MQTTDisconnect(c);
    net_disconnect_exit(c);

_mqtt_exit:
    LOG_I("MQTT server is disconnected.");

    return;
}
```

  在线程paho_mqtt_thread 内完成MQTT 连接的建立和保活、主题的订阅、网络连接c->sock 和管道pub_pipe[0] 端口的监听、消息的发布、MQTT 接收报文的处理等工作，关键节点的实现逻辑已经在上面注释清楚了，还剩下一个MQTT 接收报文处理函数MQTT_cycle 未介绍，该函数的实现代码如下：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\packages\pahomqtt-v1.1.0\MQTTClient-RT\paho_mqtt_pipe.c

static int MQTT_cycle(MQTTClient *c)
{
    // read the socket, see what work is due
    int packet_type = MQTTPacket_readPacket(c);

    int len = 0,
        rc = PAHO_SUCCESS;

    if (packet_type == -1)
    {
        rc = PAHO_FAILURE;
        goto exit;
    }
	/* 根据接收到 MQTT 报文类型的不同，分别做不同的处理 */
    switch (packet_type)
    {
    case CONNACK:
    case PUBACK:
    /* 当接收到 MQTT SUBACK 报文，由函数MQTTDeserialize_suback 解析该报文，并返回Reason Code */
    case SUBACK:
    {
        int count = 0, grantedQoS = -1;
        unsigned short mypacketid;

        if (MQTTDeserialize_suback(&mypacketid, 1, &count, &grantedQoS, c->readbuf, c->readbuf_size) == 1)
            rc = grantedQoS; // 0, 1, 2 or 0x80

        if (rc != 0x80)
            rc = 0;

        break;
    }
    /* 当接收到 MQTT UNSUBACK 报文，由函数MQTTDeserialize_unsuback 解析该报文，并返回Reason Code */
    case UNSUBACK:
    {
        unsigned short mypacketid;

        if (MQTTDeserialize_unsuback(&mypacketid, c->readbuf, c->readbuf_size) == 1)
            rc =  PAHO_SUCCESS;
        else
            rc =  PAHO_FAILURE;

        break;
    }
    /* 当接收到 PUBLISH 报文，由函数MQTTDeserialize_publish 解析该报文，如果解析成功，则调用函数deliverMessage 
       处理接收到的MQTTMessage 和topic，实际上就是调用我们注册的messageHandlers[i] 或defaultMessageHandler 消息处理回调函数，
       如果接收消息的QoS 为 1 则通过函数MQTTSerialize_ack 构造 MQTT PUBACK 报文，若接收消息的QoS 为 2 则构造 PUBREC 报文，
       最后通过函数sendPacket（实际上调用send） 将构造的PUBACK 或PUBREC 报文发送出去 */
    case PUBLISH:
    {
        MQTTString topicName;
        MQTTMessage msg;
        int intQoS;
        if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
                                    (unsigned char **)&msg.payload, (int *)&msg.payloadlen, c->readbuf, c->readbuf_size) != 1)
            goto exit;
        msg.qos = (enum QoS)intQoS;
        deliverMessage(c, &topicName, &msg);
        if (msg.qos != QOS0)
        {
            if (msg.qos == QOS1)
                len = MQTTSerialize_ack(c->buf, c->buf_size, PUBACK, 0, msg.id);
            else if (msg.qos == QOS2)
                len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREC, 0, msg.id);
            if (len <= 0)
                rc = PAHO_FAILURE;
            else
                rc = sendPacket(c, len);
            if (rc == PAHO_FAILURE)
                goto exit; // there was a problem
        }
        break;
    }
    /* 当接收到 MQTT PUBREC 报文，由函数MQTTDeserialize_ack 解析该报文，如果成功接收PUBREC 报文，
       则调用函数MQTTSerialize_ack 构造MQTT PUBREL 报文，并通过函数sendPacket（实际上调用send） 发送出去 */
    case PUBREC:
    {
        unsigned short mypacketid;
        unsigned char dup, type;
        if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
            rc = PAHO_FAILURE;
        else if ((len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREL, 0, mypacketid)) <= 0)
            rc = PAHO_FAILURE;
        else if ((rc = sendPacket(c, len)) != PAHO_SUCCESS) // send the PUBREL packet
            rc = PAHO_FAILURE; // there was a problem
        if (rc == PAHO_FAILURE)
            goto exit; // there was a problem
        break;
    }
    case PUBCOMP:
        break;
    /* 当接收到MQTT PINGRESP 报文，在c->tick_ping 中记录当前时间，作为下一次保活间隔计算的起点 */
    case PINGRESP:
        c->tick_ping = rt_tick_get();
        break;
    }

exit:
    return rc;
}

static int deliverMessage(MQTTClient *c, MQTTString *topicName, MQTTMessage *message)
{
    int i;
    int rc = PAHO_FAILURE;

    // we have to find the right message handler - indexed by topic
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if (c->messageHandlers[i].topicFilter != 0 && (MQTTPacket_equals(topicName, (char *)c->messageHandlers[i].topicFilter) ||
                isTopicMatched((char *)c->messageHandlers[i].topicFilter, topicName)))
        {
            if (c->messageHandlers[i].callback != NULL)
            {
                MessageData md;
                NewMessageData(&md, topicName, message);
                c->messageHandlers[i].callback(c, &md);
                rc = PAHO_SUCCESS;
            }
        }
    }

    if (rc == PAHO_FAILURE && c->defaultMessageHandler != NULL)
    {
        MessageData md;
        NewMessageData(&md, topicName, message);
        c->defaultMessageHandler(c, &md);
        rc = PAHO_SUCCESS;
    }

    return rc;
}
```

函数MQTT_cycle 也算是一个有限状态机，根据接收报文类型的不同，分别作出不同的处理。跟我们调用paho-mqtt API 编写应用程序关系较大的是，当接收到MQTT PUBLISH 报文时，将里面的有效消息message 和主题topic 交由函数deliverMessage 来处理，实际上就是根据topic 匹配结果，调用我们注册的消息处理回调函数messageHandlers[i] 或defaultMessageHandler。至于该如何处理收到的消息，由我们自己实现的消息处理函数而定，我们需要将实现的消息处理函数注册到MQTTClient。

## 2.2 Paho-MQTT 移植与示例
从paho-mqtt 的实现代码看，它底层调用的是RT-Thread 提供的SAL_Socket 抽象层接口，sal_socket 下面是lwip，我们在前面的工程中已经完成了sal_socket 的移植，所以paho-mqtt 底层调用接口已经准备好了，不需要再做什么移植工作。主要是上层应用如何调用paho-mqtt 接口，通过MQTT 协议完成消息传递，paho-mqtt 提供的API 如下：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\packages\pahomqtt-v1.1.0\MQTTClient-RT\paho_mqtt.h

/* subscribe topic receive data callback */
typedef void (*subscribe_cb)(MQTTClient *client, MessageData *data);

/** This function start a mqtt worker thread.
 * @param client the pointer of MQTT context structure
 * @return the error code, 0 on start successfully.
 */
int paho_mqtt_start(MQTTClient *client);

/** This function stop mqtt worker thread and free mqtt client object.
 * @param client the pointer of MQTT context structure
 * @return the error code, 0 on start successfully.
 */
int paho_mqtt_stop(MQTTClient *client);

/** This function send an MQTT subscribe packet and wait for suback before returning.
 * @param client the pointer of MQTT context structure
 * @param qos MQTT Qos type, only support QOS1
 * @param topic topic filter name
 * @param callback the pointer of subscribe topic receive data function
 * @return the error code, 0 on start successfully.
 */
int paho_mqtt_subscribe(MQTTClient *client, enum QoS qos, const char *topic, subscribe_cb callback);

/** This function send an MQTT unsubscribe packet and wait for unsuback before returning.
 * @param client the pointer of MQTT context structure
 * @param topic topic filter name
 * @return the error code, 0 on start successfully.
 */
int paho_mqtt_unsubscribe(MQTTClient *client, const char *topic);

/** This function publish message to specified mqtt topic.
 * @param c the pointer of MQTT context structure
 * @param qos MQTT QOS type, only support QOS1
 * @param topic topic filter name
 * @param msg_str the pointer of send message
 * @return the error code, 0 on subscribe successfully.
 */
int paho_mqtt_publish(MQTTClient *client, enum QoS qos, const char *topic, const char *msg_str);

/** This function control MQTT client configure, such as connect timeout, reconnect interval.
 * @param c the pointer of MQTT context structure
 * @param cmd control configure type, 'mqttControl' enumeration shows the supported configure types.
 * @param arg the pointer of argument
 * @return the error code, 0 on subscribe successfully.
 */
int paho_mqtt_control(MQTTClient *client, int cmd, void *arg);
```

Paho-mqtt 组件包提供了示例代码（见mqtt_sample.c）和测试代码（见mqtt_test.c），我们引入paho-mqtt 组件包是为onenet 组件包提供MQTT 协议支持的，这里就直接使用paho-mqtt 组件包提供的示例代码mqtt_sample.c 测试一下我们引入的paho-mqtt 组件包是否能正常工作。

前面我们在menuconfig 中已经选中了“Enable MQTT example”，也已经通过“scons --target=mdk5” 命令生成了Keil MDK5 工程文件，打开MDK5 编译完成，并将代码烧录到我们的pandora 开发板中，连接wifi 后执行mqtt_start 命令，结果如下：
![mqtt_start 连接错误](https://img-blog.csdnimg.cn/20210515184051967.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
MQTT 连接失败了，这是怎么回事儿呢？可以正常ping 通，但MQTT 连接失败，是我们移植paho-mqtt 出了问题呢？还是我们要连接的“tcp://iot.eclipse.org:1883” 出了问题呢？

我们使用前篇博文介绍的mosquitto_sub 和mosquitto_pub 命令尝试连接“iot.eclipse.org:1883”，结果如下：

```bash
# MQTT Client 1
> mosquitto_sub -t "/mqtt/test" -h "iot.eclipse.org" -p 1883 -u "admin" -P "admin"
Error: Connection refused

# MQTT Client 2
> mosquitto_pub -t "/mqtt/test" -m "Hello, IoT!" -h "iot.eclipse.org" -p 1883 -u "admin" -P "admin"
Error: Connection refused
```

由此可见，是我们要连接的MQTT Broker “iot.eclipse.org:1883”拒绝了我们的连接，我们就换前篇博客使用的“test.mosquitto.org:1883” 重新尝试。修改mqtt_sample.c 文件中的宏定义MQTT_URI 的值如下：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\packages\pahomqtt-v1.1.0\samples\mqtt_sample.c

#define MQTT_URI                "tcp://test.mosquitto.org:1883"
```

重新编译并烧录工程代码，MQTT 通信测试结果如下：
![mqtt_start 连接成功并发布消息](https://img-blog.csdnimg.cn/20210515190818999.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
这次可以成功连接MQTT Broker / Server 了，我们向订阅的主题“/mqtt/test” 发布消息“Hello, IoT!”，成功调用了我们注册的消息处理函数mqtt_sub_callback，并打印出了topic 和message。

我们再次订阅主题“/mqtt/paho”，发现订阅失败，提示MAX_MESSAGE_HANDLERS 值为1，可订阅主题数量受限。如果想订阅更多的主题，需要将宏定义MAX_MESSAGE_HANDLERS 设置为我们需要的主题数量（也即消息处理函数的数量）。

到这里，我们往工程中添加的paho-mqtt 组件包工作正常，接下来我们可以使用onenet 组件包接入OneNET 云平台了。

# 三、怎么实现远程监控和OTA 升级？
前面也提到，本文选用RT-Thread 提供的[onenet 组件包](https://github.com/RT-Thread-packages/onenet/blob/master/README_ZH.md)，我们先通过menuconfig 命令将onenet 组件包添加进我们的工程中。添加OneNET 组件包时，需要配置我们在OneNET 云平台创建的产品和设备信息，比如product id、master/product apikey、device id、authentication information、device apikey 等，配置界面如下：
![添加OneNET 组件包到我们的工程中](https://img-blog.csdnimg.cn/20210515193106262.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
我们先分析下OneNET 组件包代码的实现逻辑，再做移植和应用开发工作。

## 3.1 OneNET SDK 实现逻辑

 - **OneNET Device 数据结构**

前面定义了跟产品和设备相关的宏，OneNET SDK 中也有描述设备的数据结构如下：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\packages\onenet-latest\src\onenet_mqtt.c

struct onenet_device
{
    struct rt_onenet_info *onenet_info;

    void(*cmd_rsp_cb)(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size);

} onenet_mqtt;

// .\RT-Thread_Projects\projects\stm32l475_mqtt_sample\packages\onenet-latest\inc\onenet.h

#define ONENET_SERVER_URL              "tcp://183.230.40.39:6002"

struct rt_onenet_info
{
    char device_id[ONENET_INFO_DEVID_LEN];
    char api_key[ONENET_INFO_APIKEY_LEN];

    char pro_id[ONENET_INFO_PROID_LEN];
    char auth_info[ONENET_INFO_AUTH_LEN];

    char server_uri[ONENET_INFO_URL_LEN];

};
typedef struct rt_onenet_info *rt_onenet_info_t;
```

结构体onenet_device 主要包含rt_onenet_info 和cmd_rsp_cb 两个成员变量，rt_onenet_info 包含device_id、api_key、pro_id、auth_info、server_uri 等信息，前面四个是我们在menuconfig 中配置的，onenet sdk 会从我们配置的宏定义中读取并赋值给onenet_device 相关成员变量，server_uri 则会从宏定义ONENET_SERVER_URL 获取，默认值为“tcp://183.230.40.39:6002”。cmd_rsp_cb 是一个命令响应回调函数，用来处理OneNET 云平台下发的命令，会被注册到paho-mqtt 中MQTTClient 的defaultMessageHandler。

 - **OneNET 接入云平台并响应下发命令的实现逻辑**

OneNET SDK 是从用户调用函数onenet_mqtt_init 接入云平台的，该函数的实现代码如下：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\packages\onenet-latest\src\onenet_http.c

/** onenet mqtt client init.
 * @param   NULL
 * @return  0 : init success
 *         -1 : get device info fail
 *         -2 : onenet mqtt client init fail
 */
int onenet_mqtt_init(void)
{
    int result = 0;

    if (init_ok)
    {
        LOG_D("onenet mqtt already init!");
        return 0;
    }
	/*  将menuconfig 中配置的产品和设备信息宏，复制到onenet_info 全局变量中，供onenet SDK 其它函数访问 */
    if (onenet_get_info() < 0)
    {
        result = -1;
        goto __exit;
    }

    onenet_mqtt.onenet_info = &onenet_info;
    onenet_mqtt.cmd_rsp_cb = RT_NULL;
    
	/* 配置MQTTClient，并调用函数paho_mqtt_start 开始创建MQTT 连接、订阅主题、监听端口、发布消息等 */
    if (onenet_mqtt_entry() < 0)
    {
        result = -2;
        goto __exit;
    }

__exit:
    if (!result)
    {
        LOG_I("RT-Thread OneNET package(V%s) initialize success.", ONENET_SW_VERSION);
        init_ok = RT_TRUE;
    }
    else
        LOG_E("RT-Thread OneNET package(V%s) initialize failed(%d).", ONENET_SW_VERSION, result);

    return result;
}

static rt_err_t onenet_mqtt_entry(void)
{
    MQTTPacket_connectData condata = MQTTPacket_connectData_initializer;
    
	/* 配置MQTT Server URI、Client ID 为device id、User name 为product id、Password 为authentication info 等 */
    mq_client.uri = onenet_info.server_uri;
    memcpy(&(mq_client.condata), &condata, sizeof(condata));
    mq_client.condata.clientID.cstring = onenet_info.device_id;
    mq_client.condata.keepAliveInterval = 30;
    mq_client.condata.cleansession = 1;
    mq_client.condata.username.cstring = onenet_info.pro_id;
    mq_client.condata.password.cstring = onenet_info.auth_info;
	
	/* 为要发送或接收的MQTT 数据报文分配内存，分别为buf 和readbuf 分配2K Byte 缓存空间 */
    mq_client.buf_size = mq_client.readbuf_size = 1024 * 2;
    mq_client.buf = (unsigned char *) ONENET_CALLOC(1, mq_client.buf_size);
    mq_client.readbuf = (unsigned char *) ONENET_CALLOC(1, mq_client.readbuf_size);
    if (!(mq_client.buf && mq_client.readbuf))
    {
        LOG_E("No memory for MQTT client buffer!");
        return -RT_ENOMEM;
    }

    /* registered callback */
    mq_client.connect_callback = mqtt_connect_callback;
    mq_client.online_callback = mqtt_online_callback;
    mq_client.offline_callback = mqtt_offline_callback;

	/* 为MQTT Client 注册默认的消息处理函数mqtt_callback */
    mq_client.defaultMessageHandler = mqtt_callback;

	/* 创建paho_mqtt_thread，以完成MQTT 连接建立和保活、消息监听和处理等任务，前面已经介绍过了 */
    paho_mqtt_start(&mq_client);

    return RT_EOK;
}
```

函数onenet_mqtt_init 并没有订阅任何主题topic，配置好MQTT Broker / Server 的URI、MQTT Client ID、User name、Password 等信息后，注册了一个默认的消息处理函数mqtt_callback，当接收到来自MQTT Broker 的消息后执行mqtt_callback。MQTT Client 没有订阅任何topic，怎么接收消息呢？

MQTT Client 订阅主题实际上是在MQTT Broker 维护一个关联该Client ID 的订阅主题列表，OneNET 云平台默认为每个接入的设备维护了一个topic：”$creq/cmduuid”，方便设备接收来自OneNET 云平台的下发命令，这一点从函数mqtt_callback 的实现代码中也可以看出：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\packages\onenet-latest\src\onenet_http.c

static void mqtt_callback(MQTTClient *c, MessageData *msg_data)
{
    size_t res_len = 0;
    uint8_t *response_buf = RT_NULL;
    char topicname[45] = { "$crsp/" };		// OneNET 为每个接入的设备订阅的主题”$creq/cmduuid”，都是以"$crsp/" 开头的

    RT_ASSERT(c);
    RT_ASSERT(msg_data);

    LOG_D("topic %.*s receive a message", msg_data->topicName->lenstring.len, msg_data->topicName->lenstring.data);

    LOG_D("message length is %d", msg_data->message->payloadlen);
	
	/* 如果命令响应函数onenet_mqtt.cmd_rsp_cb 非空，当接收到来自OneNET 云平台的下发命令后，会调用执行我们注册的命令响应函数cmd_rsp_cb，
	   同时将命令响应消息response_buf 发布到接收命令的主题 */
    if (onenet_mqtt.cmd_rsp_cb != RT_NULL)
    {
        onenet_mqtt.cmd_rsp_cb((uint8_t *) msg_data->message->payload, msg_data->message->payloadlen, &response_buf,
                &res_len);

        if (response_buf != RT_NULL || res_len != 0)
        {
            strncat(topicname, &(msg_data->topicName->lenstring.data[6]), msg_data->topicName->lenstring.len - 6);

            onenet_mqtt_publish(topicname, response_buf, strlen((const char *)response_buf));

            ONENET_FREE(response_buf);
        }
    }
}

/** set the command responses call back function
 * @param   cmd_rsp_cb  command responses call back function
 * @return  0 : set success
 *         -1 : function is null
 */
void onenet_set_cmd_rsp_cb(void (*cmd_rsp_cb)(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size))
{
    onenet_mqtt.cmd_rsp_cb = cmd_rsp_cb;
}
```

只需要调用函数onenet_set_cmd_rsp_cb 就可以注册我们实现的命令响应回调函数，处理从OneNET 下发的命令了。因此，应用开发的主要任务之一就是实现并注册命令响应函数，处理从OneNET 下发的命令并返回响应数据。接入设备怎么向OneNET 云平台上传数据点呢？

 - **OneNET 设备向云平台上传数据点的实现逻辑**

MQTT Client 向MQTT Broker 上传数据自然需要用到PUBLISH 报文，也即函数paho_mqtt_publish。有两个问题：一个是该向什么topic 发布数据？第二是以怎样的数据格式发布数据？

首先看OneNET SDK 中对数据点的定义和几个上传数据点的接口函数声明：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\packages\onenet-latest\inc\onenet.h

#define ONENET_DATASTREAM_NAME_MAX     32

/* onenet datastream info */
struct rt_onenet_ds_info
{
    char id[ONENET_DATASTREAM_NAME_MAX];
    char tags[ONENET_DATASTREAM_NAME_MAX];

    char update_time[ONENET_DATASTREAM_NAME_MAX];
    char create_time[ONENET_DATASTREAM_NAME_MAX];

    char unit[ONENET_DATASTREAM_NAME_MAX];
    char unit_symbol[ONENET_DATASTREAM_NAME_MAX];

    char current_value[ONENET_DATASTREAM_NAME_MAX];

};
typedef struct rt_onenet_ds_info *rt_onenet_ds_info_t;


/* Publish MQTT digit data to onenet. */
rt_err_t onenet_mqtt_upload_digit(const char *ds_name, const double digit);

/* Publish MQTT string data to onenet. */
rt_err_t onenet_mqtt_upload_string(const char *ds_name, const char *str);

/* Publish MQTT binary data to onenet. */
rt_err_t onenet_mqtt_upload_bin(const char *ds_name, uint8_t *bin, size_t len);

#ifdef RT_USING_DFS
/* Publish MQTT binary data to onenet by path. */
rt_err_t onenet_mqtt_upload_bin_by_path(const char *ds_name, const char *bin_path);
#endif
```

结构体rt_onenet_ds_info 定义的属性比较多，我们看OneNET SDK 代码主要是从云平台获取数据流模板时用到了，从本地向云平台上传数据点并没有用到rt_onenet_ds_info 数据类型。考虑到我们要上传的温湿度数据比较简单，可以暂不考虑数据类型rt_onenet_ds_info。

从本地设备向OneNET 云平台上传数据主要有数值类型、字符串类型、二进制类型、二进制文件类型等，我们要上传的是温湿度数据，属于浮点数类型，因此可以通过调用函数onenet_mqtt_upload_digit 实现温湿度数据点的上传，该函数的实现代码如下：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\packages\onenet-latest\src\onenet_mqtt.c

#define  ONENET_TOPIC_DP    "$dp"

/** Upload digit data to OneNET cloud.
 * @param   ds_name     datastream name
 * @param   digit       digit data
 * @return  0 : upload digit data success
 *         -5 : no memory
 */
rt_err_t onenet_mqtt_upload_digit(const char *ds_name, const double digit)
{
    char *send_buffer = RT_NULL;
    rt_err_t result = RT_EOK;
    size_t length = 0;

    RT_ASSERT(ds_name);

	/* 将数据点名称ds_name 和数值digit 构造为Json 格式 2 字符串，也即OneNET 定义的数据类型 3 */
    result = onenet_mqtt_get_digit_data(ds_name, digit, &send_buffer, &length);
    if (result < 0)
        goto __exit;

	/* 构造MQTTMessage 数据类型，并调用函数MQTTPublish 将消息发布到主题"$dp"，与函数paho_mqtt_publish 功能一致 */
    result = onenet_mqtt_publish(ONENET_TOPIC_DP, (uint8_t *)send_buffer, length);
    if (result < 0)
    {
        LOG_E("onenet publish failed (%d)!", result);
        goto __exit;
    }

__exit:
    if (send_buffer)
        ONENET_FREE(send_buffer);

    return result;
}
```

从函数onenet_mqtt_upload_digit 的实现代码可知，向OneNET 上传数据点的主题是"\$dp"，这也是OneNET 云平台定义的，我们需要上传的数据点按要求构造好Json 数据类型后，将数据点消息发布到主题"\$dp"即可。

至于怎样构造Json 数据类型呢？限于篇幅，这里就不展开介绍了，使用的是[cJSON 组件库，github 该组件库的主页](https://github.com/DaveGamble/cJSON)有详细介绍，也给出了示例。JSON 是一种简单且常用的结构化数据格式，比XML 或HTML 格式更简单，主要以key-value 为基础组织起来的数据结构。

函数onenet_mqtt_get_digit_data 的功能是将数据点名称ds_name 和浮点型数值digit 构造为如下的Json 数据类型（这个Json 数据只包含一个key-value 元素，算是最简单的了，若想了解更复杂的可以参阅[Wikipedia: JSON](http://en.wiki.sxisa.org/wiki/JSON)）：

```javascript
{
	“ds_name”:digit
}
```

要上传的数据点格式，除了要求数据内容为Json 格式外，还有前三个字节需要填充，首字节是第几个数据类型，本文选择的是数据类型 3，后两个字节填写该Json 格式数据的长度。上传数据点类型的构造过程在函数onenet_mqtt_get_digit_data 中可以看到，该函数的实现代码如下：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\packages\onenet-latest\src\onenet_mqtt.c

static rt_err_t onenet_mqtt_get_digit_data(const char *ds_name, const double digit, char **out_buff, size_t *length)
{
    rt_err_t result = RT_EOK;
    cJSON *root = RT_NULL;
    char *msg_str = RT_NULL;

    RT_ASSERT(ds_name);
    RT_ASSERT(out_buff);
    RT_ASSERT(length);

    root = cJSON_CreateObject();
    if (!root)
    {
        LOG_E("MQTT publish digit data failed! cJSON create object error return NULL!");
        return -RT_ENOMEM;
    }

    cJSON_AddNumberToObject(root, ds_name, digit);

    /* render a cJSON structure to buffer */
    msg_str = cJSON_PrintUnformatted(root);
    if (!msg_str)
    {
        LOG_E("MQTT publish digit data failed! cJSON print unformatted error return NULL!");
        result = -RT_ENOMEM;
        goto __exit;
    }

    *out_buff = ONENET_MALLOC(strlen(msg_str) + 3);
    if (!(*out_buff))
    {
        LOG_E("ONENET mqtt upload digit data failed! No memory for send buffer!");
        return -RT_ENOMEM;
    }

    strncpy(&(*out_buff)[3], msg_str, strlen(msg_str));
    *length = strlen(&(*out_buff)[3]);

    /* mqtt head and json length */
    (*out_buff)[0] = 0x03;
    (*out_buff)[1] = (*length & 0xff00) >> 8;
    (*out_buff)[2] = *length & 0xff;
    *length += 3;

__exit:
    if (root)
        cJSON_Delete(root);

    if (msg_str)
        cJSON_free(msg_str);

    return result;
}
```

到这里我们就了解本地设备接入OneNET，实现上传数据点并响应下发命令，编写应用程序的关键了。首先需要调用函数onenet_mqtt_init 让本地设备接入OneNET 云平台，其次实现并注册命令响应函数onenet_mqtt.cmd_rsp_cb 来响应OneNET 云平台下发的命令，最后通过调用函数onenet_mqtt_upload_digit 完成温湿度数据的上传。接下来，我们编写应用程序，实现上述功能。

## 3.2 向OneNET 上传温湿度数据点
首先，需要在 lwip 准备就绪后，调用函数onenet_mqtt_init 让本地设备接入OneNET 云平台，我们可以在WLAN 框架内注册事件RT_WLAN_EVT_READY（表示已连接wifi 且lwip 已就绪，可以发送数据了）的回调函数，在该回调函数内调用函数onenet_mqtt_init，我们可以在main.c 文件中添加如下代码：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\applications\main.c
......
#include <onenet.h>
......
void wlan_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    if(onenet_mqtt_init() != 0)
    {
        LOG_E("RT-Thread OneNET package(V%s) initialize failed.", ONENET_SW_VERSION);
        return;
    }
}

int main(void)
{
    /* 注册 wlan 回调函数 */
    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wlan_ready_handler, RT_NULL);

    /* 初始化 wlan 自动连接配置 */
    wlan_autoconnect_init();
    /* 使能 wlan 自动连接功能 */
    rt_wlan_config_autoreconnect(RT_TRUE);
    ......
    return 0;
}
```

一般上传数据点需要周期性连续上传，最好创建一个线程专门用来上传温湿度数据，我们按照这种思路在main.c 中实现上传温湿度数据的代码如下（温湿度传感器的初始化在博文[Sensor管理框架](https://blog.csdn.net/m0_37621078/article/details/103115383)中已完成注册）：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\applications\main.c
......
#include <sensor.h>

/* defined aht10 sensor name */
#define SENSOR_TEMP_NAME    "temp_aht10"
#define SENSOR_HUMI_NAME    "humi_aht10"
......
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

        rt_thread_delay(rt_tick_from_millisecond(5 * 1000));
    }

    rt_device_close(sensor_temp);
}

/* RT_WLAN_EVT_READY 事件回调函数 */
static void wlan_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    rt_thread_t tid;

    /* 初始化OneNET 组件包 */
    ......
    /* 等待MQTT 连接建立成功，也可以放到mqtt_online_callback 中创建上传数据点的线程 */
    rt_thread_delay(rt_tick_from_millisecond(5 * 1000));

    /* 创建周期性上传数据点的线程 */
    tid = rt_thread_create("onenet_upload_datapoint",
                           onenet_upload_datapoint_thread,
                           RT_NULL,
                           2 * 1024,
                           RT_THREAD_PRIORITY_MAX / 3 - 1,
                           5);
    if (tid)
        rt_thread_startup(tid);
}
......
```

我们使用命令”scons --target=mdk5“ 生成Keil MDK5 工程，编译工程成功，将代码烧录到我们的Pandora 开发板中。我们启用了wifi 自动连接功能，首次配网成功连接wifi 热点后，会将wifi 热点SSID 和password 存储到W25Q128 Flash 中，下次重启或者重新烧录代码，只要周围的wifi 热点信息不变，就可以自动从flash 读取 wifi 热点信息并连接。由于之前的示例工程我们也启用了wifi 自动连接并完成了wifi 配网，因此这里不需要再重新输入wifi 连接信息，pandora 开发板上传数据点的log 信息输出如下：
![上传数据点串口log](https://img-blog.csdnimg.cn/20210516193706735.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
OneNET 云平台查看数据流展示，发现温湿度数据确实上传成功了：
![向OneNET 云平台上传的温湿度数据点](https://img-blog.csdnimg.cn/20210516193945147.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)

## 3.3 响应OneNET 下发的LED 控制命令
我们的开发板向OneNET 云平台上传数据点成功了，说明本地设备成功接入了云平台，我们的鉴权信息配置没问题。实现了上传数据点功能，接下来如何实现响应下发命令的功能呢？

我们先通过命令控制LED 灯的亮灭，注册一个命令响应回调函数，在main.c 文件中添加如下代码：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\applications\main.c
......
/* defined the LED_R pin: PE7 */
#define LED_R    GET_PIN(E, 7)
......
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

    /* user have to malloc memory for response data */
    *resp_data = (uint8_t *) ONENET_MALLOC(strlen(res_buf));

    strncpy((char *)*resp_data, (const char *)res_buf, strlen(res_buf));

    *resp_size = strlen(res_buf);
}

/* RT_WLAN_EVT_READY 事件回调函数 */
static void wlan_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    ......
    /* 创建周期性上传数据点的线程 */
    ......
    /* 注册命令响应回调函数 */
    onenet_set_cmd_rsp_cb(onenet_cmd_rsp_callback);
}
```

重新编译工程，并将代码烧录到pandora 开发板中，从OneNET 云平台下发命令“led-on”，可以看到开发板上的LED 灯确实亮了，再下发命令“led-off”，开发板上的LED 灯又灭了，串口输出的log 数据如下：
![响应OneNET 下发的命令](https://img-blog.csdnimg.cn/2021051620105299.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)

## 3.4  响应OneNET 的OTA 升级命令
前面博文我们实现了OTA 升级功能，将升级固件放到我们自己搭建的服务器上，然后通过串口发送命令执行OTA 升级过程。既然需要通过串口发送命令，远程OTA 升级功能相比有线升级的优势就弱了很多，能否让开发板真正实现远程升级呢？

既然我们的开发板已经能够响应OneNET 下发的命令了，自然也可以响应云平台下发的OTA 升级命令，命令可以包含待升级固件的服务器地址，在main.c 文件中添加如下代码：

```c
// .\RT-Thread_Projects\projects\stm32l475_onenet_sample\applications\main.c
......
/* onenet mqtt command response callback function */
static void onenet_cmd_rsp_callback(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size)
{
    ......
    /* 命令匹配 */
    if (rt_strncmp((const char *)recv_data, "led-on", 6) == 0)
    {
        /* 开灯 */
        ......
    }
    else if (rt_strncmp((const char *)recv_data, "led-off", 7) == 0)
    {
        /* 关灯 */
        ......
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
```

编译工程，并将代码烧录到开发板中。同时更改版本号（将宏APP_VERSION 修改为3.0.0），重新编译用于升级的固件，使用 rt_ota_packaging_tool 工具打包用于升级的新固件“rtthread.rbl”。使用MyWebServer 创建一个存储升级固件的服务器，配置服务器 IP、Port、新版固件“rtthread.rbl” 所在目录等参数，然后启动固件托管服务器：
![启动升级固件托管服务器](https://img-blog.csdnimg.cn/20210516211939750.png)
启动固件托管服务器后，我们可以发送命令`http_ota "http://192.168.43.145:80/rtthread.rbl"` 开始OTA 升级过程，我们在OneNET云平台下发该命令，通过串口可以看到开发板按照预期开始了固件升级过程：
![OneNET 下发命令开始OTA 升级过程](https://img-blog.csdnimg.cn/20210516214445186.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
上面这种远程OTA 升级功能需要使用第三方固件托管服务器，各大物联网云平台都为接入的设备提供了远程OTA 升级功能，不过大多需要收费，OneNET 自然也不例外，有兴趣的读者可以借助OneNET 提供的远程OTA 升级功能实现远程固件升级。




# 更多文章：
 - 《[Web技术（七）：如何使用并实现MQTT 消息订阅-发布模型？](https://blog.csdn.net/m0_37621078/article/details/116246058)》
 - 《[Web技术（六）：QUIC 是如何解决TCP 性能瓶颈的？](https://blog.csdn.net/m0_37621078/article/details/106506532)》
 - 《[Web技术（五）：HTTP/2 是如何解决HTTP/1.1 性能瓶颈的？](https://blog.csdn.net/m0_37621078/article/details/106006303)》
 - 《[Web技术（四）：TLS 握手过程与性能优化(TLS 1.2与TLS 1.3对比)](https://blog.csdn.net/m0_37621078/article/details/106126033)》
 - 《[Web技术（三）：TLS 1.2/1.3 加密原理(AES-GCM + ECDHE-ECDSA/RSA)](https://blog.csdn.net/m0_37621078/article/details/106028622)》
 - 《[Web技术（二）：图解HTTP + HTTPS + HSTS](https://blog.csdn.net/m0_37621078/article/details/105662287)》
 - 《[Web技术（一）：互联网的设计与演化(URL + HTML + HTTP)](https://blog.csdn.net/m0_37621078/article/details/105543208)》
 - 《[如何使用HTTP协议实现OTA空中升级](https://github.com/StreamAI/RT-Thread_Projects/tree/master/projects/stm32l475_ota_sample)》

 - 《[AP6181(BCM43362) WiFi模块移植](https://github.com/StreamAI/RT-Thread_Projects/tree/master/projects/stm32l475_wifi_sample)》
 - 《[ESP8266 WiFi模块移植](https://github.com/StreamAI/LwIP_Projects/tree/master/stm32l475-pandora-wifi)》
 - 《[LwIP 协议栈移植](https://github.com/StreamAI/LwIP_Projects/tree/master/stm32l475-pandora-lwip)》