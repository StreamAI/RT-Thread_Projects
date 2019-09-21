#include <rtthread.h>

#define THREAD_STACK_SIZE   1024
#define THREAD_PRIORITY     20
#define THREAD_TIMESLICE    10

static rt_thread_t tid1 = RT_NULL;

ALIGN(RT_ALIGN_SIZE)
static char thread2_stack[1024];
static struct rt_thread thread2;


/* 线程入口 */
static void thread_entry(void* parameter)
{
    rt_uint32_t value;
    rt_uint32_t count = 0;
    rt_thread_t thread = RT_NULL;

    value = (rt_uint32_t)parameter;
    
    while (1)
    {
        if(0 == (count % 15))
        {
            thread = rt_thread_self();

            rt_enter_critical();            
            rt_kprintf("%s is running ,thread %d count = %d\n", thread->name , value , count);
            rt_exit_critical();        
                
            if(count > 80)
            {
                if(thread == tid1)
                    rt_thread_suspend(thread);
                
                rt_thread_mdelay(5000);
                return;
            }
        }
        count++;
     }
}

static void hook_of_scheduler(struct rt_thread* from, struct rt_thread* to)
{
    if(from == tid1 || from == &thread2)
        rt_kprintf("scheduler from: %s -->  to: %s. \n", from->name , to->name);
}


static int thread_sample(void)
{
    /* 设置调度器钩子 */
    rt_scheduler_sethook(hook_of_scheduler);

    /* 创建线程 1 */
    tid1 = rt_thread_create("thread1",
                            thread_entry, (void*)1,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);


    /* 初始化线程 2 */
    rt_thread_init(&thread2, "thread2",
                    thread_entry, (void*)2,
                    &thread2_stack[0], sizeof(thread2_stack),
                    THREAD_PRIORITY, THREAD_TIMESLICE-5);
    rt_thread_startup(&thread2);

    return 0;
}

/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(thread_sample, thread sample);
