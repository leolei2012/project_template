#include "hal.h"
#include "hal_gpio.h"
#include "bsp_config.h"

#include <rtthread.h>

#include "drv.h"

const uint32_t g_version = 0x010000u;  /**< 主版本号(高字节) + 次版本号(低字节) + 修订号(第三字节) */

static void idle_hook(void)
{
    hal_gpio_write(HAL_GPIO_LED_RED, HAL_GPIO_LEVEL_LOW);
}

static void led_on_callback(void)
{
    hal_gpio_write(HAL_GPIO_LED_RED, HAL_GPIO_LEVEL_HIGH);
}





int main(void)
{

    /// rt_thread_idle_sethook(idle_hook);


    while (1)
    {



        hal_gpio_toggle(HAL_GPIO_LED_RED);
        rt_thread_mdelay(500);
    }
}