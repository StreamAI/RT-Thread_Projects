#include "rtthread.h"
#include "rtdevice.h"
#include "board.h"

/* 引脚编号，通过查看STM32L475参考手册获知 */
#define BEEP_PIN    GET_PIN(B, 2)

#define KEY0_PIN    GET_PIN(D, 10)
#define KEY1_PIN    GET_PIN(D, 9)
#define KEY2_PIN    GET_PIN(D, 8)

#define WKUP_PIN    GET_PIN(C, 13)

static void beep_on(void *args)
{
    rt_kprintf("turn on beep!\n");

    rt_pin_write(BEEP_PIN, PIN_HIGH);
}

static void beep_off(void *args)
{
    rt_kprintf("turn off beep!\n");

    rt_pin_write(BEEP_PIN, PIN_LOW);
}

static void pin_beep_sample(void)
{
    /* 蜂鸣器引脚配置为输出模式 */
    rt_pin_mode(BEEP_PIN, PIN_MODE_OUTPUT);
    /* 默认低电平 */
    rt_pin_write(BEEP_PIN, PIN_LOW);

     /* 按键0引脚配置为上拉输入模式 */
    rt_pin_mode(KEY0_PIN, PIN_MODE_INPUT_PULLUP);
    /* 绑定中断回调函数，下降沿触发模式，回调函数名为beep_on */
    rt_pin_attach_irq(KEY0_PIN, PIN_IRQ_MODE_FALLING, beep_on, RT_NULL);
    /* 使能中断 */
    rt_pin_irq_enable(KEY0_PIN, PIN_IRQ_ENABLE);

    /* 按键1引脚配置为上拉输入模式 */
    rt_pin_mode(KEY1_PIN, PIN_MODE_INPUT_PULLUP);
    /* 绑定中断回调函数，下降沿触发模式，回调函数名为beep_off */
    rt_pin_attach_irq(KEY1_PIN, PIN_IRQ_MODE_FALLING, beep_off, RT_NULL);
    /* 使能中断 */
    rt_pin_irq_enable(KEY1_PIN, PIN_IRQ_ENABLE);
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(pin_beep_sample, pin_beep sample);
