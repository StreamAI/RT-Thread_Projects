#include <rtthread.h>

static int memheap_sample(void)
{
    int i = 0;
    rt_uint8_t *ptr[10];

    for (i = 0; i < 10; i ++)
        ptr[i] = RT_NULL;
    /* 每次分配 (4096 * i) 大小字节数的内存空间 */
    for(i = 1; i < 10; i++){
        if(ptr[i-1] == RT_NULL){
            ptr[i-1] = rt_malloc(4096 * i);
            if(ptr[i-1] != RT_NULL){
                rt_kprintf("malloc memory heap address: %X, size: %d bytes.\n",ptr[i-1], (4096 * i));
            }else{
                rt_kprintf("try to malloc %d bytes memory failed.\n",(4096 * i));
                break;
            }
        }
    }
    /* 释放内存块 */
    for(i = 0; i < 10; i++){
        if(ptr[i] != RT_NULL){
            rt_kprintf("free memory heap address: %X, size: %d bytes.\n", ptr[i], (4096 * i));
            rt_free(ptr[i]);
            ptr[i] = RT_NULL;
        }else{
            rt_kprintf("free memory heap finished.\n");
            break;
        }       
    }
    return RT_EOK;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(memheap_sample, memheap sample);


#define THREAD_PRIORITY      25
#define THREAD_STACK_SIZE    512
#define THREAD_TIMESLICE     5

#define MP_BLOCK_COUNT      4
#define MP_BLOCK_SIZE       1024

static rt_uint8_t *ptr[5];
static rt_mp_t mp;

/* 指向线程控制块的指针 */
static rt_thread_t tid1 = RT_NULL;
static rt_thread_t tid2 = RT_NULL;

/* 线程 1 入口 */
static void thread1_mp_alloc(void *parameter)
{
    int i;

    for (i = 0 ; i < 5 ; i++)
    {
        if (ptr[i] == RT_NULL)
        {
            /* 试图申请内存块 50 次，当申请不到内存块时，线程 1 挂起，转至线程 2 运行 */
            ptr[i] = rt_mp_alloc(mp, RT_WAITING_FOREVER);
            if (ptr[i] != RT_NULL)
                rt_kprintf("allocate memory pool address: %X, block No.%d\n", ptr[i], i);
        }
    }
}

/* 线程 2 入口，线程 2 的优先级比线程 1 低，应该线程 1 先获得执行。*/
static void thread2_mp_release(void *parameter)
{
    int i;

    for (i = 0; i < 5 ; i++)
    {
        /* 释放所有分配成功的内存块 */
        if (ptr[i] != RT_NULL)
        {
            rt_kprintf("release memory pool address: %X, block NO.%d\n", ptr[i], i);
            rt_mp_free(ptr[i]);
            ptr[i] = RT_NULL;
        }
    }
    rt_thread_mdelay(5000);
    rt_mp_delete(mp);
}

static int mempool_sample(void)
{
    int i;

    for (i = 0; i < 5; i ++)
        ptr[i] = RT_NULL;

    /* 创建内存池对象 */
    mp = rt_mp_create("mp1", MP_BLOCK_COUNT, MP_BLOCK_SIZE);
    if(mp == RT_NULL){
        rt_kprintf("memory pool create failed.\n");
        return RT_ERROR;
    }

    /* 创建线程 1：申请内存池 */
    tid1 = rt_thread_create("thread1", thread1_mp_alloc, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);


    /* 创建线程 2：释放内存池 */
    tid2 = rt_thread_create("thread2", thread2_mp_release, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY + 1, THREAD_TIMESLICE);
    if (tid2 != RT_NULL)
        rt_thread_startup(tid2);

    return 0;
}

/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(mempool_sample, mempool sample);
