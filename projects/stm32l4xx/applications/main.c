/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <stdlib.h>

/* defined the LED_R/LED_G/LED_B pin: PE7/PE8/PE9 */
#define LED_R    GET_PIN(E, 7)
#define LED_G    GET_PIN(E, 8)
#define LED_B    GET_PIN(E, 9)

/* defined RGB LED Color enum */
typedef enum{
	RED,
	GREEN,
	BLUE,
	YELLOW,
	PURPLE,
	CYAN,
	WHITE,
	BLACK,
	MAX_NUM
}RGB_Color;


int RGB_LED_init(void)
{
    /* set LED_R/LED_G/LED_B pin mode to output */
    rt_pin_mode(LED_R, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_G, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_B, PIN_MODE_OUTPUT);

    return 0;
}

INIT_APP_EXPORT(RGB_LED_init);

void RGB_ON(RGB_Color RGB_LED)
{
	switch(RGB_LED % MAX_NUM)
		{
			case RED:
				rt_pin_write(LED_R, PIN_LOW);
				rt_pin_write(LED_G, PIN_HIGH);
				rt_pin_write(LED_B, PIN_HIGH);
        		rt_kprintf("red led on.\n");
				break;
			case GREEN:
				rt_pin_write(LED_R, PIN_HIGH);
				rt_pin_write(LED_G, PIN_LOW);
				rt_pin_write(LED_B, PIN_HIGH);
        		rt_kprintf("green led on.\n");
				break;
			case BLUE:
				rt_pin_write(LED_R, PIN_HIGH);
				rt_pin_write(LED_G, PIN_HIGH);
				rt_pin_write(LED_B, PIN_LOW);
        		rt_kprintf("blue led on.\n");
				break;
			case YELLOW:
				rt_pin_write(LED_R, PIN_LOW);
				rt_pin_write(LED_G, PIN_LOW);
				rt_pin_write(LED_B, PIN_HIGH);
        		rt_kprintf("yellow led on.\n");
				break;
			case PURPLE:
				rt_pin_write(LED_R, PIN_LOW);
				rt_pin_write(LED_G, PIN_HIGH);
				rt_pin_write(LED_B, PIN_LOW);
        		rt_kprintf("purple led on.\n");
				break;
			case CYAN:
				rt_pin_write(LED_R, PIN_HIGH);
				rt_pin_write(LED_G, PIN_LOW);
				rt_pin_write(LED_B, PIN_LOW);
        		rt_kprintf("cyan led on.\n");
				break;
			case WHITE:
				rt_pin_write(LED_R, PIN_LOW);
				rt_pin_write(LED_G, PIN_LOW);
				rt_pin_write(LED_B, PIN_LOW);
        		rt_kprintf("white led on.\n");
				break;
			default:
				rt_pin_write(LED_R, PIN_HIGH);
				rt_pin_write(LED_G, PIN_HIGH);
				rt_pin_write(LED_B, PIN_HIGH);
        		rt_kprintf("led off.\n");
				break;
		}
}

int main(void)
{
    unsigned int count = 0;

    while (count < 8)
    {
        RGB_ON(count);
        rt_thread_mdelay(1000);
		count++;
    }

    return RT_EOK;
}

static int RGB_Control(int argc, char **argv)
{
	if(argc != 2){
		rt_kprintf("Please input 'RGB_ON <0-7>'\n");
		return -1;
	}

	RGB_ON(atoi(argv[1]));

	return 0;
}

MSH_CMD_EXPORT_ALIAS(RGB_Control,RGB,RGB Sample: RGB <0-7>);
