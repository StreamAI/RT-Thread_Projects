#include <rtthread.h>
/* 定时器控制块的句柄指针 */
static rt_timer_t timer1;
/* 定时器的控制块 */
static struct rt_timer timer2;

static int count = 0;

/* 定时器 1 超时函数 */
static void timeout1(void *parameter)
{
    rt_kprintf("periodic timer is timeout %d\n", count);

    /* 运行第 10 次，停止周期定时器 */
    if (count++ >= 9)
    {
        rt_timer_stop(timer1);      /* 停止定时器 1 */
        rt_kprintf("periodic timer was stopped! \n");
    }
}

/* 定时器 2 超时函数 */
static void timeout2(void *parameter)
{
    rt_tick_t tick2;
    /* 控制定时器2 获取超时tick值 */
    rt_timer_control(&timer2,               /* 定时器句柄 */
                    RT_TIMER_CTRL_GET_TIME, /* 定时器控制命令 */
                    (void *)&tick2);        /* 定时器控制参数 */

    rt_kprintf("one shot timer is timeout, get tick is %d\n",tick2);
}

int timer_sample(void)
{
    /* 创建定时器 1  周期定时器 */
    timer1 = rt_timer_create("timer1",      /* 定时器名字是 timer1 */
                            timeout1,       /* 超时回调的处理函数 */
                            RT_NULL,        /* 超时函数的入口参数 */
                            10,             /* 定时长度为 10 个 OS Tick */
                            RT_TIMER_FLAG_PERIODIC);    /* 周期定时器 */

    /* 启动定时器 1 */
    if (timer1 != RT_NULL)
        rt_timer_start(timer1);

    /* 创建定时器 2 单次定时器 */
    rt_timer_init(&timer2,  /* 定时器句柄是 &timer2 */
                "timer2",   /* 定时器名字是 timer2 */
                timeout2, /* 超时回调的处理函数 */
                RT_NULL, /* 超时函数的入口参数 */
                30,     /* 定时长度为 30 个 OS Tick */
                RT_TIMER_FLAG_ONE_SHOT); /* 单次定时器 */

    /* 启动定时器 2 */
    rt_timer_start(&timer2);

    return 0;
}

/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(timer_sample, timer sample);