# 如何使用HTTP协议实现OTA空中升级？

## 一、Bootloader OTA 原理
随着物联网技术的普及，越来越多的嵌入式产品支持网络访问能力，嵌入式产品接入网络，可以方便的从云端获得云计算和人工智能的支持。嵌入式产品不仅可以将复杂的运算过程放到服务器端完成，还可以接受经过训练的人工智能模型的协调，实现与其它嵌入式产品协同高效配合，提供智能化场景服务的能力。

这些被赋予人工智能支持的嵌入式产品可以称为智能硬件，智能硬件为了不断优化与其它智能硬件的高效配合，也为了不断扩展支持的服务场景，需要具备自我迭代升级的能力。在博文：[ARM 代码烧录方案与原理详解](https://blog.csdn.net/m0_37621078/article/details/106798909)中已经介绍过代码烧录与升级的各种方案，既然智能硬件具备网络访问能力，使用OTA 空中升级技术实现智能硬件Application 代码的升级迭代更加便捷，一键升级的操作对用户也更加友好。

OTA 空中升级技术需要开发者自己实现Bootloader 代码，不过主流的IOT 操作系统开发商已经为我们提供了Bootloader 的开发框架，我么只需要在此基础上根据自己需要进行适量修改即可，大大简化了开发Bootloader 的工作量。RT-Thread 便为我们提供了通用的Bootloader 的软件框架，开发者可以通过该Bootloader 直接使用RT-Thread OTA 功能，轻松实现对设备端固件的管理、升级与维护。
![Bootloader 框架](https://img-blog.csdnimg.cn/20200622235216975.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
RT-Thread 提供的Bootloader 软件框架，底层由Flash 驱动提供ROM 或Flash 分区访问的能力。博文[ARM 代码烧录方案与原理详解](https://blog.csdn.net/m0_37621078/article/details/106798909)中介绍过，OTA 空中升级需要本地提供部分存储区间，Bootloader 有一个重要功能就是搬移固件代码，比如升级固件代码时需要从Download 分区读取待升级的固件代码，经校验通过后，写入或搬移到Application 分区覆盖正在使用的固件代码，这就完成了固件升级过程。

我们在前篇博文：[WLAN管理框架 + AP6181(BCM43362) WiFi模块](https://blog.csdn.net/m0_37621078/article/details/105264345)工程中FAL 分区的基础上增加bootloader 分区，更新后的分区表如下：

```c
// projects\stm32l475_ota_sample\ports\fal\fal_cfg.h

#define NOR_FLASH_DEV_NAME "W25Q128"
/* partition table */
#define FAL_PART_TABLE                                                                                                  \
{                                                                                                                       \
    {FAL_PART_MAGIC_WROD, "bootloader",     "onchip_flash",                                    0,        64 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD,        "app",     "onchip_flash",                            64 * 1024,       448 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD,  "easyflash", NOR_FLASH_DEV_NAME,                                    0,       512 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD,   "download", NOR_FLASH_DEV_NAME,                           512 * 1024,      1024 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD, "wifi_image", NOR_FLASH_DEV_NAME,                  (512 + 1024) * 1024,       512 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD,       "font", NOR_FLASH_DEV_NAME,            (512 + 1024 + 512) * 1024,  7 * 1024 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD, "filesystem", NOR_FLASH_DEV_NAME, (512 + 1024 + 512 + 7 * 1024) * 1024,  7 * 1024 * 1024, 0}, \
}
```

上面的分区表中，bootloader 分区和app（application的简称）分区位于片上Flash 的Main Flash memory 存储区间，download 分区位于片外Flash 的W25Q128 上（由于STM32L475 片上Flash 空间只有512KB，需要片外Flash 扩展存储空间），download 分区用于暂存Application 代码更新软件包。片外Flash上的wifi_image 分区用于存储AP6181 WIFI 模块的固件代码，Bootloader 同样可以将暂存在download 分区内的WIFI 模块更新固件包搬移到wifi_image 分区内，实现WIFI 模块固件的升级。

Bootloader 除了提供访问Flash 分区，在不同分区之间搬移固件代码的功能外，还提供了固件加解密、固件解压缩的功能。由于智能硬件是连接Internet 的，这就有可能遭遇网络攻击，比如固件升级包被截获并篡改等，为了应对网络攻击，Bootloader 提供了将固件升级包进行加密认证传输的功能（可以参考博文：[TLS 1.2/1.3 加密原理](https://blog.csdn.net/m0_37621078/article/details/106028622)）。为了减少传输开销，同时减少对存储空间的占用，Bootloader 提供了将固件升级包进行压缩传输的功能，如果固件更新代码占比较小，还可以以差分升级的方式提高效率。OTA 技术中Bootloader 提供的主要功能如下：

 -  **固件加密**：支持AES-256 加密算法，提高固件下载、存储安全性；
 - **固件防篡改**：使用HMAC（Hash Message Authentication Code，算是哈希摘要算法比如SHA-256 的进阶版）校验固件包的完整性，如果固件被篡改将无法通过完整性校验，保证了固件传输、存储的安全可靠；
 - **固件压缩**：支持Quicklz 和Fastlz 等压缩算法，固件经过高效压缩，可节省传输流量，减少Flash 空间占用，降低下载时间；
 - **差分升级**：根据版本差异生成差分包（常采用多bin 文件升级方式，每次只升级其中的少数bin 文件），进一步节省Flash 空间，节省传输流量，加快升级速度；
 - **断电保护**：可将升级进度与状态同步记录到ROM中，即便遇到意外断电中止升级过程，也可在上电重启后从ROM 读取升级进度和状态继续完成升级过程，减少返厂维修概率；
 - **智能还原**：支持将出厂固件或前一个稳定版本的固件存储到recovery 分区，当运行中的固件损坏时，可以将recovery 分区中的代码搬移到Applicaion 分区，相当于恢复到出厂固件版本或者回退到前一个稳定版本固件，保证设备的可靠运行。

为了减少Bootloader 代码的复杂度，将固件升级包下载过程放到Aplication 代码中完成了，毕竟通过Internet 下载固件升级包需要TCP/IP 协议栈（包括MAC层的LTE、WLAN、WPAN协议栈和应用层的HTTP、FTP协议栈等）的支持，这些网络协议栈代码还是挺占用存储空间的。

放到Application 代码中的OTA Downloader 组件也是OTA 空中升级技术的一个重要组成部分，Bootloader 部分主要实现固件升级包的校验、解压缩、解密、代码搬运等功能。OTA 空中升级技术需要的两大组件：OTA Downloader 和Bootloader 层级框架图示如下：
![RT-OTA 框架](https://img-blog.csdnimg.cn/20200623150702872.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
OTA Downloader 组件主要是将固件升级包下载到特定存储分区，比如片外Flash 的Download 分区，供Bootloader 从该分区读取、检验固件升级包。OTA Downloader 组件可以支持通过USB 通讯协议（借助Y-modem 组件）从本地PC 下载固件升级包，也支持通过HTTP 协议（借助http client 组件）从特定服务器下载固件升级包，从固定云端服务器（借助RT-Cloud OTA 组件）下载固件升级包实际使用的还是HTTP 协议，只是提供了更便捷友好的交互界面。

OTA Bootloader 组件主要提供了通过FAL 组件访问Flash 分区的功能，便于从Download 分区读取固件升级包，同时将固件代码搬移到目标存储区间。为了提高固件升级包传输、存储的安全性，Bootloader 还提供了Tinycrypt 加密功能（使用AES-256 + HMAC-SHA256算法 ）。为了降低传输开销、减小存储空间占用，Bootloader 还提供了Quicklz 或Fastlz 解压缩组件，这些组件都是可选的。

在嵌入式系统方案里，要完成一次OTA 固件远端升级，通常需要以下阶段：
![OTA 固件升级流程](https://img-blog.csdnimg.cn/20200623164451326.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
 1. 准备固件升级文件（RT-Thread 使用ota_packager 打包生成 .rbl 格式的固件升级文件），并上传OTA 固件升级文件到固件托管服务器；
 2. 设备端使用对应的OTA Downloader 组件从固件托管服务器下载OTA 固件升级文件到本地Download 分区；
 3. 新版本固件下载完成后，在适当的时候重启进入Bootloader；
 4. Bootloader 对本地Download 分区内的OTA 固件升级文件进行解密、解压缩、校验等操作（详细流程可参考下图），如果校验通过则将新版本固件代码搬运到app 分区（如果是WiFi 固件升级文件则搬运到wifi_image 分区）；
 5. 升级成功，执行新版本app 固件。

![Bootloader OTA 升级流程](https://img-blog.csdnimg.cn/20200625000618610.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
 RT-Thread 提供的STM32 Bootloader 是闭源的，本文也没法对其实现原理进行过多介绍。我们可以通过网页端[http://iot.rt-thread.com](http://iot.rt-thread.com)在线生成的方式获取，根据自己使用的芯片填写相关参数，就可以生成自己芯片可用的bootloader.bin 文件，生成过程可参考博文：[STM32 通用 Bootloader](https://www.rt-thread.org/document/site/application-note/system/rtboot/an0028-rtboot/)。
![为Pandora开发板生成Bootloader 配置参数](https://img-blog.csdnimg.cn/20200623232849212.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
先看硬件配置部分，只支持SPI Flash，并不支持QSPI 通信协议，Pandora 开发板与W25Q128 Flash是通过QSPI 引脚连接的，这里如果只能配置SPI 引脚的话，就只能使用QSPI 接口的单端SPI 引脚了（可参考博文：[SPI + QSPI + HAL](https://blog.csdn.net/m0_37621078/article/details/101395150)），传输速率比较慢。再看分区表配置，只能配置app、download、factory 三个分区，无法为WIFI 模块更新固件。

在线生成的Bootloader 虽然能够使用，但扩展性较弱，使用SPI 协议搬运代码速度较慢，不能访问W25Q128 Flash 的全部分区。本文我们使用潘多拉STM32L475 开发板光盘资料中提供的bootloader.bin 文件，将Pandora IoT 例程中该文件的路径复制到我们工程目录的路径如下：

```c
// 潘多拉STM32L475 开发板光盘资料中bootloader.bin 文件路径
.\RT-Thread IoT例程\examples\23_iot_ota_http\bin\bootloader.bin

// bootloader.bin 文件拷贝到我们工程中的目标路径
.\projects\stm32l475_ota_sample\bin\bootloader.bin
```

使用“STM32 ST-LINK Utility” 工具分别将我们通过网页在线生成的rtboot_l4.bin 和从Pandora IoT 例程拷贝来的bootloader.bin 烧录到Pandora 开发板中，启动界面对比如下（左图是rtboot_l4.bin 的启动界面，右图是bootloader.bin 的启动界面）：
![网页生成的和Pandora附带的Bootloader 对比](https://img-blog.csdnimg.cn/20200624000047323.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
从上面左右图对比可以看出，Pandora IoT 例程中的bootloader.bin 针对STM32L475 开发板做了更多的适配修改，可以访问上文给出的分区表中的全部分区，通过升级速度对比，猜测这个bootloader.bin 也是支持QSPI 通讯协议的。
![通过ST-LINK Utility 烧录bootloader 文件步骤](https://img-blog.csdnimg.cn/20200624005229354.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
我们已经有了bootloader.bin，并将其烧录到Pandora 开发板内的bootloader 分区（该分区起始地址与大小上文已给出，烧录方法如上图示），接下来就该准备Application 升级文件了，Application 代码包含OTA Downloader 组件，下面介绍OTA Downloader 组件的实现原理。

## 二、HTTP OTA Downloader 实现
物联网时代，嵌入式产品越来越多的具备网络访问能力，这类产品常通过OTA 空中升级技术完成固件版本更新。不管是通过蜂窝移动网、WLAN、WPAN等无线接入方式访问Internet，还是通过Ethernet 等有线接入方式访问Internet，主要都是借助网络应用层的HTTP 协议获取固件升级包的（也有通过FTP 协议获取的，本文使用HTTP 协议）。

RT-Thread 提供的OTA Downloader 组件有两种固件下载方式：

 - **http_ota**：通过HTTP 协议获取固件升级文件，支持通过LTE、WiFi、Bluetooth 等无线网络下载固件升级文件；
 - **ymodem_ota**：通过ymodem 协议获取固件升级文件，实际是通过UART 有线接口下载固件升级文件。

本文主要介绍http_ota 方式下载固件升级文件的原理，由于使用了HTTP 协议，还需要webclient 组件提供HTTP 协议支持。如果读者不了解HTTP 协议，可以先阅读博文：[图解HTTP + HTTPS + HSTS](https://blog.csdn.net/m0_37621078/article/details/105662287)。

我们先在env 环境中执行menuconfig 命令，到“RT-Thread online packages" --> "IoT - internet of things” 菜单下，分别启用“WebClient ” 组件（启用文件下载功能）和“ota_downloader” 组件（启用HTTP/HTTPS OTA，并配置默认的URL ），配置界面如下：
![启用webclient 组件与ota_downloader 组件](https://img-blog.csdnimg.cn/20200624102219457.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
HTTP/HTTPS OTA Downloader 实际上就是使用HTTP 协议下载固件升级文件，对比."\ota_downloader-v1.0.0\src\http_ota.c" 与“.\webclient-v2.1.2\src\webclient_file.c” 文件的实现原理是类似的，只是前者将下载的文件保存到了FAL 存储分区，后者将下载的文件保存到文件系统中，二者都是使用HTTP/HTTPS 从远端服务器获取文件资源的。我们先从HTTP WebClient 的代码实现开始介绍，HTTP 协议理论部分参考博文：[图解HTTP + HTTPS + HSTS](https://blog.csdn.net/m0_37621078/article/details/105662287)。

 - **HTTP Session 数据结构描述**

WebClient - v2.1.2 只实现了HTTP/1.1 的GET 与POST 方法，对于我们从远端服务器获取固件升级文件已经够用了。HTTP 数据报文主要由请求行/响应行、首部字段、空行、报文主体几部分构成，其中的报文主体有可能长度很大，不适合放到HTTP Session 结构体内，因此HTTP Session 主要包括请求行/响应行、首部字段、报文主体长度等信息。由于HTTP 是基于TCP 通信的，TCP/IP 协议对上层提供了一组Socket API，HTTP Session 也应包含socket 信息。WebClient 组件给出的HTTP Session 数据结构定义如下：

```c
// projects\stm32l475_ota_sample\packages\webclient-v2.1.2\inc\webclient.h

struct webclient_session
{
    struct webclient_header *header;    /* webclient response header information */
    int socket;
    int resp_status;

    char *host;                         /* server host */
    char *req_url;                      /* HTTP request address*/

    int chunk_sz;
    int chunk_offset;

    int content_length;
    size_t content_remainder;           /* remainder of content length */

    rt_bool_t is_tls;                   /* HTTPS connect */
#ifdef WEBCLIENT_USING_MBED_TLS
    MbedTLSSession *tls_session;        /* mbedtls connect session */
#endif
};

struct  webclient_header
{
    char *buffer;
    size_t length;                      /* content header buffer size */

    size_t size;                        /* maximum support header size */
};
```
webclient_header 数据结构描述比较简单，相当于就定义了一段缓存区，用户将HTTP 的首部字段按ASCII 编码及固定格式（每个首部字段后面跟回车换行符）拼接到一起即可，HTTP 报文的请求行、响应行、空行也以首部字段的形式保存在webclient_header 结构体中。webclient_session 结构体包含webclient_header 结构体指针、socket、响应状态码、请求URL / 服务Host、分块传输chunk_xxx、报文主体长度content_length 等信息，还为TLS 的支持留下扩展成员tls_session。

webclient 组件除了支持HTTP/1.x，还可以配合MbedTLS 组件进行加密传输（也即HTTPS）。由于MbedTLS 占用存储资源较大，本文尝试使用该组件编译工程时提示存储空间不足（除去Bootloader 的64KB 空间，留给Application 的只剩448KB，MbedTLS 要占用超过100KB），再加上本文要用的HTTP 服务器"MyWebServer" 使用HTTPS 功能需要的openssl库找不到可下载的资源，本文就不使用HTTPS 来传输固件升级文件了，仅使用较简单且占用资源较少的HTTP/1.1 来传输固件升级文件。

 - **WebClient 接口函数及调用关系**

WebClient 既然是HTTP 协议的一种实现，向上层提供的API 自然是请求和响应，由于响应是对请求的应答，所以上层可以通过一个接口函数webclient_request 向服务器发送请求报文并处理接收到的响应报文（WebClient 组件仅支持HTTP/1.x 的GET 与POST 两种请求方法），WebClient 组件向上层提供的API --- webclient_request 的函数实现代码如下：

```c
// projects\stm32l475_ota_sample\packages\webclient-v2.1.2\src\webclient.c
/**
 *  send request(GET/POST) to server and get response data.
 *
 * @param URI input server address
 * @param header send header data
 *             = NULL: use default header data, must be GET request
 *            != NULL: user custom header data, GET or POST request
 * @param post_data data sent to the server
 *             = NULL: it is GET request
 *            != NULL: it is POST request
 * @param response response buffer address
 *
 * @return <0: request failed
 *        >=0: response buffer size
 */
int webclient_request(const char *URI, const char *header, const char *post_data, unsigned char **response)
{
    struct webclient_session *session = RT_NULL;
    int rc = WEBCLIENT_OK;
    int totle_length = 0;

    RT_ASSERT(URI);
    if (post_data == RT_NULL && response == RT_NULL)
        return -WEBCLIENT_ERROR;

    if (post_data == RT_NULL)
    {
        /* send get request */
        session = webclient_session_create(WEBCLIENT_HEADER_BUFSZ);
        if (session == RT_NULL)
        {
            rc = -WEBCLIENT_NOMEM;
            goto __exit;
        }

        if (header != RT_NULL)
        {
            char *header_str, *header_ptr;
            int header_line_length;

            for(header_str = (char *)header; (header_ptr = rt_strstr(header_str, "\r\n")) != RT_NULL; )
            {
                header_line_length = header_ptr + rt_strlen("\r\n") - header_str;
                webclient_header_fields_add(session, "%.*s", header_line_length, header_str);
                header_str += header_line_length;
            }
        }

        if (webclient_get(session, URI) != 200)
        {
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }

        totle_length = webclient_response(session, response);
        if (totle_length <= 0)
        {
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }
    }
    else
    {
        /* send post request */
        session = webclient_session_create(WEBCLIENT_HEADER_BUFSZ);
        if (session == RT_NULL)
        {
            rc = -WEBCLIENT_NOMEM;
            goto __exit;
        }

        if (header != RT_NULL)
        {
            char *header_str, *header_ptr;
            int header_line_length;

            for(header_str = (char *)header; (header_ptr = rt_strstr(header_str, "\r\n")) != RT_NULL; )
            {
                header_line_length = header_ptr + rt_strlen("\r\n") - header_str;
                webclient_header_fields_add(session, "%.*s", header_line_length, header_str);
                header_str += header_line_length;
            }
        }

        if (rt_strstr(session->header->buffer, "Content-Length") == RT_NULL)
            webclient_header_fields_add(session, "Content-Length: %d\r\n", rt_strlen(post_data));

        if (rt_strstr(session->header->buffer, "Content-Type") == RT_NULL)
            webclient_header_fields_add(session, "Content-Type: application/octet-stream\r\n");

        if (webclient_post(session, URI, post_data) != 200)
        {
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }

        totle_length = webclient_response(session, response);
        if (totle_length <= 0)
        {
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }
    }

__exit:
    if (session)
    {
        webclient_close(session);
        session = RT_NULL;
    }

    if (rc < 0)
        return rc;

    return totle_length;
}
```

函数webclient_request 有四个参数，分别是请求资源的 URL、首部字段指针 *header、要发送给服务器的POST 请求报文的报文主体数据指针 *post_data、接收到的服务器响应报文的报文主体数据缓冲区地址 *response（也即请求到的资源数据的存储地址），客户端采用GET 还是POST 请求方法发送请求报文，取决于第三个参数是否为空指针。

请求报文中要设置哪些首部字段可以使用接口函数webclient_header_fields_add() 添加相应的字段名称和字段值， webclient 组件也为我们提供了几个默认字段：请求行、Host 字段、User-Agent 字段、空行等，如果我们不设置任何首部字段，将只使用这几个默认首部字段构造请求报文。

WebClient 组件属于应用层HTTP 协议，通信依赖于下层的TCP 协议，因此WebClient 向服务器请求资源的过程，底层是由Socket API 实现的。上层webclient_request() 接口函数到底层Socket API 的调用关系如下图所示：
![WebClient API 调用关系](https://img-blog.csdnimg.cn/20200624155412396.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
上面只展示了HTTP 协议的接口函数调用关系，由于本文没有使用mbedtls 组件，就没有将mbedtls 的接口函数放到上图中，即便使用mbedtls 组件，调用逻辑也跟上面类似。了解了HTTP 协议后，上图的中间函数理解起来并不难，限于篇幅这里就不再一一介绍了。

Http_ota_downloader 实现过程跟上图中的webclient_get_file 函数实现过程类似，二者的区别是前者通过HTTP 协议将文件下载到FAL 存储分区，后者通过HTTP 协议将文件下载到文件系统中（需要先在一个Block 设备上创建文件系统）。webclient_get_file 函数的实现过程跟前面介绍的webclient_request 函数实现过程类似，由于主要下载文件，相比webclient_request 函数更简单些，只需要两个参数，为方便下文介绍Http_ota_downloader 的实现过程，这里给出webclient_get_file 函数的实现代码以供对比（限于篇幅，删除了部分代码）：

```c
// projects\stm32l475_ota_sample\packages\webclient-v2.1.2\src\webclient_file.c
/**
 * send GET request and store response data into the file.
 *
 * @param URI input server address
 * @param filename store response date to filename
 *
 * @return <0: GET request failed
 *         =0: success
 */
int webclient_get_file(const char* URI, const char* filename)
{
    int fd = -1, rc = WEBCLIENT_OK;
    size_t offset;
    int length, total_length = 0;
    unsigned char *ptr = RT_NULL;
    struct webclient_session* session = RT_NULL;
    int resp_status = 0;

    session = webclient_session_create(WEBCLIENT_HEADER_BUFSZ);
    if(session == RT_NULL)
    ......
    if ((resp_status = webclient_get(session, URI)) != 200)
    ......
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd < 0)
    ......
    ptr = (unsigned char *) web_malloc(WEBCLIENT_RESPONSE_BUFSZ);
    if (ptr == RT_NULL)
    ......
    if (session->content_length < 0)
    {
        while (1)
        {
            length = webclient_read(session, ptr, WEBCLIENT_RESPONSE_BUFSZ);
            if (length > 0)
            {
                write(fd, ptr, length);
                total_length += length;
                LOG_RAW(">");
            }
            else
                break;
        }
    }
    else
    {
        for (offset = 0; offset < (size_t) session->content_length;)
        {
            length = webclient_read(session, ptr,
                    session->content_length - offset > WEBCLIENT_RESPONSE_BUFSZ ?
                            WEBCLIENT_RESPONSE_BUFSZ : session->content_length - offset);

            if (length > 0)
            {
                write(fd, ptr, length);
                total_length += length;
                LOG_RAW(">");
            }
            else
                break;
                
            offset += length;
        }
    }

__exit:
    if (fd >= 0)
        close(fd);
        
    if (session != RT_NULL)
        webclient_close(session);

    if (ptr != RT_NULL)
        web_free(ptr);

    return rc;
}

int wget(int argc, char** argv)
{
    if (argc != 3)
    {
        rt_kprintf("Please using: wget <URI> <filename>\n");
        return -1;
    }
    webclient_get_file(argv[1], argv[2]);
    return 0;
}
MSH_CMD_EXPORT(wget, Get file by URI: wget <URI> <filename>.);
```

Webclient 组件还为webclient_get_file 函数导出了一个MSH 命令，我们在工程中添加webclient 组件后，可以待网络连接成功后，使用wget 命令从某个URL 下载一个文件到本地filesystem 分区（本文基于前一篇博文的工程：[AP6181(BCM43362) WiFi模块驱动移植](https://blog.csdn.net/m0_37621078/article/details/105264345)，在该工程中已经为filesystem 分区创建了一个elmfat 文件系统），如果能顺利从远端服务器下载一个文件到本地，并且该文件是可以正常访问的，说明webclient 组件的添加和配置没有问题。
![wget 示例结果](https://img-blog.csdnimg.cn/2020062420110018.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)


 - **HTTP_ota_downloader 实现原理**

HTTP_ota_downloader 组件的核心就是 http_ota_fw_download 函数，前面也说了该函数的实现过程与webclient_get_file 函数类似，二者最大的不同就是访问Flash 存储分区的方式不同，前面已经展示了webclient_get_file 函数的实现代码，下面展示http_ota_fw_download 函数的实现代码如下：

```c
// projects\stm32l475_ota_sample\packages\ota_downloader-v1.0.0\src\http_ota.c

#define HTTP_OTA_URL              PKG_HTTP_OTA_URL

static int http_ota_fw_download(const char* uri)
{
    int ret = 0, resp_status;
    int file_size = 0, length, total_length = 0;
    rt_uint8_t *buffer_read = RT_NULL;
    struct webclient_session* session = RT_NULL;
    const struct fal_partition * dl_part = RT_NULL;

    /* create webclient session and set header response size */
    session = webclient_session_create(GET_HEADER_BUFSZ);
    if (!session)
    ......
    /* send GET request by default header */
    if ((resp_status = webclient_get(session, uri)) != 200)
    ......
    file_size = webclient_content_length_get(session);
    rt_kprintf("http file_size:%d\n",file_size);
    if (file_size <= 0)
    ......
    /* Get download partition information and erase download partition data */
    if ((dl_part = fal_partition_find("download")) == RT_NULL)
    {
        LOG_E("Firmware download failed! Partition (%s) find error!", "download");
        ret = -RT_ERROR;
        goto __exit;
    }

    if (fal_partition_erase(dl_part, 0, file_size) < 0)
    {
        LOG_E("Firmware download failed! Partition (%s) erase error!", dl_part->name);
        ret = -RT_ERROR;
        goto __exit;
    }

    buffer_read = web_malloc(HTTP_OTA_BUFF_LEN);
    if (buffer_read == RT_NULL)
    ......
    memset(buffer_read, 0x00, HTTP_OTA_BUFF_LEN);
    LOG_I("OTA file size is (%d)", file_size);

    do
    {
        length = webclient_read(session, buffer_read, file_size - total_length > HTTP_OTA_BUFF_LEN ?
                            HTTP_OTA_BUFF_LEN : file_size - total_length);   
        if (length > 0)
        {
            /* Write the data to the corresponding partition address */
            if (fal_partition_write(dl_part, total_length, buffer_read, length) < 0)
            {
                LOG_E("Firmware download failed! Partition (%s) write data error!", dl_part->name);
                ret = -RT_ERROR;
                goto __exit;
            }
            total_length += length;

            print_progress(total_length, file_size);
        }
        else
        {
            LOG_E("Exit: server return err (%d)!", length);
            ret = -RT_ERROR;
            goto __exit;
        }

    } while(total_length != file_size);

    ret = RT_EOK;

    if (total_length == file_size)
    {
        if (session != RT_NULL)
            webclient_close(session);
        if (buffer_read != RT_NULL)
            web_free(buffer_read);

        LOG_I("Download firmware to flash success.");
        LOG_I("System now will restart...");

        rt_thread_delay(rt_tick_from_millisecond(5));

        /* Reset the device, Start new firmware */
        extern void rt_hw_cpu_reset(void);
        rt_hw_cpu_reset();
    }

__exit:
    if (session != RT_NULL)
        webclient_close(session);
    if (buffer_read != RT_NULL)
        web_free(buffer_read);

    return ret;
}

void http_ota(uint8_t argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("using uri: " HTTP_OTA_URL "\n");
        http_ota_fw_download(HTTP_OTA_URL);
    }
    else
        http_ota_fw_download(argv[1]);
}
MSH_CMD_EXPORT(http_ota, Use HTTP to download the firmware);
```

对比http_ota_fw_download 函数与webclient_get_file 函数的实现代码也可以看出其中的相似性，调用webclient 组件接口函数的过程基本一致，http_ota_fw_download 函数直接将下载的固件升级文件存储到FAL 的download 分区，不需要为该分区创建一个文件系统，相当于这个分区对客户是隐藏的，既节省了文件系统管理的开销，又能防止存储在download 分区中的固件被用户破坏。

需要注意的一点是，在往download 分区写入数据前，需要先将其擦除，也即将该存储分区的所有位都写为1，因为Flash 编程原理都是只能将1写为0，而不能将0写成1。http_ota_fw_download 函数为了让用户能直观感受到下载进度，还通过print_progress 函数增加了打印下载进度的功能。

当文件下载完成后，http_ota_fw_download 函数会在最后调用rt_hw_cpu_reset 函数让MCU 重启复位，开始执行bootloader 代码，bootloader 检查download 分区内有固件升级文件，且校验通过后，会将download 分区内的固件代码搬移到app 分区，完成固件版本升级，最后再跳转到app 分区执行更新后的Application 代码。

Ota_downloader 组件也为http_ota_fw_download 函数导出了一个MSH 命令，我们可以使用http_ota 命令完成从远端服务器下载固件升级文件到本地FAL download 分区的任务。在启用ota_downloader 组件时可以设置一个默认的URL（固件升级文件所在远端托管服务器的URL），如果想换个下载源URL，只需要使用`http_ota <URL> `，也即在命令后加一个URL 参数即可。

## 三、Bootloader OTA示例
到这里Bootloader 代码已经准备好了，OTA Downloader 模块也已经添加进Application 了，可以继续第一部分介绍的OTA 固件远端升级方案了吗？再回顾下博文：[ARM 代码烧录方案与原理详解](https://blog.csdn.net/m0_37621078/article/details/106798909)中介绍的IAP 烧录方案，由于Application 代码前面要为Bootloader 代码预留存储空间，也即Application 代码存储在Main Flash memory 区间起始位置向后偏移一段距离处，需要重新设置中断向量表偏移地址，也即重新设置 VTOR 寄存器的值，同时修改Application 工程的ROM 区间地址参数。

本文为bootloader 分配了64KB 的存储空间，app 分区的起始地址为0x0801 0000，区间大小为448KB（也即0x70000 字节）。首先我们需要将Application 工程的中断向量表映射到app 分区起始位置也即0x0801 0000 处，该任务可以通过设置VTOR 中断向量表偏移寄存器完成，VTOR 寄存器的结构如下图示：
![VTOR寄存器结构](https://img-blog.csdnimg.cn/20200624213827524.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
从上图可知，Cortex-M4 的VTOR 寄存器bit31:7 有效，我们可以定义一个VTOR 掩码NVIC_VTOR_MASK，将有效位置1，无效位置0，得到掩码NVIC_VTOR_MASK值为0xFFFFFF80。我们再将要设置的目标偏移地址0x0801 0000 与该掩码进行位与运算，即可得到VTOR 寄存器的值。设置VTOR 寄存器的代码如下：

```c
// projects\stm32l475_ota_sample\applications\main.c

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
```

重新设置VTOR 寄存器的函数ota_app_vtor_reconfig 被自动初始化组件调用，INIT_BOARD_EXPORT 说明该函数是最早被初始化的，此时调度器还未启动。重新设置中断向量表后，系统开始启动并进入main 函数，按照正确的中断向量表响应系统异常与中断服务。

前一篇博文中主要介绍WIFI 模块的移植和使用，main 函数设计的较复杂，本文中对其简化，只对WIFI 模块进行必要的初始化配置，连接WIFI 的操作交由用户通过“`wifi join [SSID] [PASSWORD]`”命令完成，这里配置了WIFI 自动连接功能，已经连接过的WIFI 在MCU 重启后会自动连接。

既然本工程主要为了验证版本升级，我们定义一个软件版本APP_VERSION，在main 函数中打印当前的软件版本，后续升级版本时，我们同步更新版本号，就可以通过当前软件版本号来判断是否升级成功了，添加打印当前软件版本信息后的main 函数代码如下：

```c
// projects\stm32l475_ota_sample\applications\main.c

#define APP_VERSION  "1.0.0"

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
```

修改完中断向量表偏移地址，还需要修改Application 工程ROM 的地址参数，包括board.h 和linker_scripts 文件中的地址参数配置，本文使用的IDE 工具为Keil MDK，对应的linker_scripts 文件是link.sct，这两个文件修改ROM 地址参数如下：

```c
// projects\stm32l475_ota_sample\board\board.h
......
#define STM32_FLASH_START_ADRESS       ((uint32_t)0x08010000)
#define STM32_FLASH_SIZE               (448 * 1024)
#define STM32_FLASH_END_ADDRESS        ((uint32_t)(STM32_FLASH_START_ADRESS + STM32_FLASH_SIZE))
......

// projects\stm32l475_ota_sample\board\linker_scripts\link.sct

LR_IROM1 0x08010000 0x00070000  {    ; load region size_region
  ER_IROM1 0x08010000 0x00070000  {  ; load address = execution address
  ......
  }
  ......
}
```

修改完board.h 和link.sct 文件中的ROM 参数配置，当然也需要修改Keil MDK Options 中对应的ROM 参数配置，我们可以在模板文件template.uvprojx 中按下图所示修改ROM 参数配置，以后通过scons --target=mdk 命令重新生成工程时就不需要再次修改ROM 参数了。
![Keil MDK 修改ROM 地址参数配置](https://img-blog.csdnimg.cn/20200624223129159.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
到这里Application 工程就准备好了，我们在env 环境中执行`scons --target=mdk5` 命令生成MDK 工程，打开工程文件project.uvprojx 编译无报错，将代码烧录到Pandora 开发板中。本文第一部分已经将bootloader.bin 文件通过“STM32 ST-LINK Utility” 工具烧录到Pandora 开发板中了，由于Application 工程（也即本文中的stm32l475_ota_sample 工程）已经重新配置了ROM 起始地址与区间大小参数，通过Keil MDK 工具正好将Application 工程代码烧录到app 分区（也即Main Flash memory 区间中bootloader 代码后面）。Bootloader 与application 代码烧录后，Pandora 开发板bootloader 和application 的启动界面分别如下：
![stm32l475_ota_sample 工程执行结果](https://img-blog.csdnimg.cn/2020062422565741.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
Pandora 开发板easyflash 分区已存储WIFI 热点参数，自动连接WIFI 生效，使用第二部分介绍的wget 命令下载百度首页到本地文件系统成功，说明本工程新增的webclient 组件工作正常。

下面开始按照第一部分介绍的OTA 固件远端升级方案继续进行，首先是使用`rt_ota_packaging_tool` 打包生成固件更新文件，将工程中的APP_VERSION 宏定义修改为“2.0.0”，使用Keil MDK 重新编译工程，生成bin格式的工程文件rtthread.bin。使用`rt_ota_packaging_tool` 工具将文件`rtthread.bin` 打包为文件`rtthread.rbl`，`rt_ota_packaging_tool` 工具的配置界面如下：
![OTA 固件打包配置参数](https://img-blog.csdnimg.cn/20200624230953812.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
选择Keil MDK 编译生成的工程文件`rtthread.bin`，配置压缩算法、加密算法、加密密钥、加密初始化向量IV、固件分区名、固件版本等参数即可打包为bootloader 可解析的升级文件`rtthread.rbl`（默认与文件`rtthread.bin` 在相同目录下）。本文以升级application 代码作为示例，所以固件分区名填写app 分区，该bootloader 也支持升级其它分区的代码，比如升级WIFI 模块的固件`wifi_image.rbl` 时固件分区名填写wifi_image 分区即可。

接下来将生成的固件更新文件`rtthread.rbl` 上传到托管服务器，本文使用`MyWebServer`工具作为托管服务器，执行`MyWebServer.exe`程序，在服务目录项选择生成的固件更新文件`rtthread.rbl`，配置IP 地址为你使用的PC 的IP地址（可通过`ipconfig /all`命令查看），HTTP 端口号为80，启动Start 运行`MyWebServer`托管服务器：
![将升级文件托管到MyWebServer 服务器](https://img-blog.csdnimg.cn/20200624232645823.png)
在finsh 控制台执行命令`http_ota "http://192.168.43.145:80/rtthread.rbl"`即可启动Application 工程中的OTA_Downloader 模块，开始从托管服务器（也即上面启动的`MyWebServer`服务器）下载固件升级文件`rtthread.rbl`到download 分区，下载完成后MCU 重启复位开始执行bootloader 程序。Bootloader 程序对本地Download 分区内的OTA 固件升级文件`rtthread.rbl` 进行解密、解压缩、校验等操作，如果校验通过则将新版本固件代码搬运到app 分区，代码搬运完成后跳转到新版本的application 代码开始执行，整个过程如下图所示：
![OTA 升级执行结果](https://img-blog.csdnimg.cn/20200624234025992.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
执行`http_ota`命令前的软件版本号为“1.0.0”，执行`http_ota`命令并完成固件升级后，finsh 显示的软件版本号为“2.0.0”，说明已成功完成OTA 固件升级过程。




# 更多文章：

 - 《[ARM 代码烧录方案与原理详解 --- SWD/JTAG + Bootloader + OTA](https://blog.csdn.net/m0_37621078/article/details/106798909)》
 - 《[Web技术（一）：互联网的设计与演化(URL + HTML + HTTP)](https://blog.csdn.net/m0_37621078/article/details/105543208)》
 - 《[Web技术（二）：图解HTTP + HTTPS + HSTS](https://blog.csdn.net/m0_37621078/article/details/105662287)》
 - 《[Web技术（三）：TLS 1.2/1.3 加密原理(AES-GCM + ECDHE-ECDSA/RSA)](https://blog.csdn.net/m0_37621078/article/details/106028622)》
 - 《[Web技术（四）：TLS 握手过程与性能优化(TLS 1.2与TLS 1.3对比)](https://blog.csdn.net/m0_37621078/article/details/106126033)》
 - 《[Web技术（五）：HTTP/2 是如何解决HTTP/1.1 性能瓶颈的？](https://blog.csdn.net/m0_37621078/article/details/106006303)》
 - 《[Web技术（六）：QUIC 是如何解决TCP 性能瓶颈的？](https://blog.csdn.net/m0_37621078/article/details/106506532)》
 - 《[AP6181(BCM43362) WiFi模块移植](https://github.com/StreamAI/RT-Thread_Projects/tree/master/projects/stm32l475_wifi_sample)》
 - 《[ESP8266 WiFi模块移植](https://github.com/StreamAI/LwIP_Projects/tree/master/stm32l475-pandora-wifi)》
 - 《[LwIP 协议栈移植](https://github.com/StreamAI/LwIP_Projects/tree/master/stm32l475-pandora-lwip)》