#include <rtthread.h>

#define THREAD_PRIORITY       6
#define THREAD_STACK_SIZE     512
#define THREAD_TIMESLICE      5

/* 定义最大 5 个元素能够被产生 */
#define MAXNUM                5

/* 定义两个事件 */
#define EVENT_FLAG1 (1 << 1)
#define EVENT_FLAG2 (1 << 2)

/* 指向IPC控制块的指针 */
static rt_mailbox_t mb;
static rt_mq_t      mq;
static rt_event_t   event;

/* 指向线程控制块的指针 */
static rt_thread_t producer1_tid = RT_NULL;
static rt_thread_t producer2_tid = RT_NULL;
static rt_thread_t consumer_tid =  RT_NULL;

/* 生产者1线程入口 */
static void producer1_thread_entry(void *parameter)
{
    int cnt = 0;

    /* 运行 8 次 */
    while (cnt < 8)
    {
        /* 向邮箱发送 1 个数据 */
        if(rt_mb_send_wait(mb, cnt, RT_WAITING_FOREVER) == RT_EOK)
        {           
            rt_kprintf("the producer1 generates a number: %d\n", cnt);
            /* 向事件集发送事件1 */
            rt_event_send(event, EVENT_FLAG1);

            cnt++;
        }
        /* 暂停一段时间 */
        rt_thread_mdelay(20);
    }
    rt_kprintf("the producer1 exit!\n");
}

/* 生产者2线程入口 */
static void producer2_thread_entry(void *parameter)
{
    char buf = 'A';

    /* 运行 8 次 */
    while (buf < 'I')
    {
        /* 向消息队列发送 1 个数据 */
        if(rt_mq_send(mq, &buf, sizeof(buf)) == RT_EOK)
        {           
            rt_kprintf("the producer2 generates a message: %c\n", buf);
            /* 向事件集发送事件2 */
            rt_event_send(event, EVENT_FLAG2);

            buf++;
        }
        /* 暂停一段时间 */
        rt_thread_mdelay(30);
    }
    rt_kprintf("the producer2 exit!\n");
}

/* 消费者线程入口 */
static void consumer_thread_entry(void *parameter)
{
    rt_uint32_t sum = 0;
    rt_uint8_t  str[9] = {0};
    rt_uint8_t  i = 0;
    rt_uint32_t cnt;
    rt_uint8_t  buf;
    rt_uint32_t e;

    while (1)
    {
        /* 接收事件1与事件2，事件组合逻辑与，接收后清除事件 */
        if(rt_event_recv(event, (EVENT_FLAG1 | EVENT_FLAG2), 
                        RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, 
                        5000, &e) == RT_EOK)
        {
            /* 从邮箱接收 1 个数据，并从消息队列接收一个消息 */
            if(rt_mb_recv(mb, (rt_ubase_t *)&cnt, RT_WAITING_FOREVER) == RT_EOK &&
                rt_mq_recv(mq, &buf, sizeof(buf), RT_WAITING_FOREVER) == RT_EOK)
            {
                sum += cnt;
                str[i++] = buf;
                rt_kprintf("the consumer get a number and a message: (%d, %c)\n", cnt, buf);
            }        
        }

        /* 如果生产者线程退出，消费者线程相应停止 */
        if (producer1_tid->stat == RT_THREAD_CLOSE || producer2_tid->stat == RT_THREAD_CLOSE)
        {
            str[i] = '\0';
            break;
        }          
        /* 暂停一小会时间 */
        rt_thread_mdelay(50);
    }
    /* 输出消费者数据和消息处理结果 */
    rt_kprintf("the consumer sum is: %d\n", sum);
    rt_kprintf("the consumer str is: %s\n", str);
    rt_kprintf("the consumer exit!\n");
}

static int producer_consumer_ex(void)
{
    /* 创建  1 个邮箱 */
    mb = rt_mb_create("mailbox", MAXNUM, RT_IPC_FLAG_FIFO);
    if(mb == RT_NULL){
        rt_kprintf("create mailbox failed.\n");
        return -1;
    }

    /* 创建  1 个消息队列 */
    mq = rt_mq_create("messagequeue",1 , MAXNUM, RT_IPC_FLAG_FIFO);
    if(mq == RT_NULL){
        rt_kprintf("create messagequeue failed.\n");
        return -1;
    }

    /* 创建  1 个事件集 */
    event = rt_event_create("event", RT_IPC_FLAG_FIFO);
    if(event == RT_NULL){
        rt_kprintf("create event failed.\n");
        return -1;
    }

    /* 创建生产者1线程 */
    producer1_tid = rt_thread_create("producer1",
                                    producer1_thread_entry, RT_NULL,
                                    THREAD_STACK_SIZE,
                                    THREAD_PRIORITY - 1,
                                    THREAD_TIMESLICE);
    if (producer1_tid != RT_NULL){
        rt_thread_startup(producer1_tid);
    }else{
        rt_kprintf("create thread producer1 failed.\n");
        return -1;
    }

    /* 创建生产者2线程 */
    producer2_tid = rt_thread_create("producer2",
                                    producer2_thread_entry, RT_NULL,
                                    THREAD_STACK_SIZE,
                                    THREAD_PRIORITY,
                                    THREAD_TIMESLICE);
    if (producer2_tid != RT_NULL){
        rt_thread_startup(producer2_tid);
    }else{
        rt_kprintf("create thread producer2 failed.\n");
        return -1;
    }

    /* 创建消费者线程 */
    consumer_tid = rt_thread_create("consumer",
                                    consumer_thread_entry, RT_NULL,
                                    THREAD_STACK_SIZE,
                                    THREAD_PRIORITY + 1,
                                    THREAD_TIMESLICE);
    if (consumer_tid != RT_NULL){
        rt_thread_startup(consumer_tid);
    }else{
        rt_kprintf("create thread consumer failed.\n");
        return -1;
    }

    return 0;
}

/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT_ALIAS(producer_consumer_ex, pro_con_ex, producer_consumer_ex sample);

