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
const static char http_index_html[] = "<html><head><title>Sensor-AHT10</title></head>\
                                    <body><h1>Welcome to LwIP 2.1.0 HTTP server!</h1></body></html>";
static char Sensor_Data[] ="<html><head><title>Sensor-AHT10</title></head>\
                            <body><center><p>The current temperature is: %3d.%d C, humidity is: %3d.%d %.</p></center></body></html>";

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
                rt_thread_mdelay(6000);
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
